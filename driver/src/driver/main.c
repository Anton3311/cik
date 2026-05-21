#include <stdio.h>
#include <stdlib.h>

#include "core/core.h"
#include "core/profiler.h"
#include "parser/preprocessor.h"
#include "parser/parser.h"
#include "compiler/compiler.h"
#include "code_gen/backends/x64.h"

static bool enum_installed_win_sdks(String sdk_install_path,
		Arena* allocator,
		Arena* temp_allocator,
		StringArray* out_sdks) {

	*out_sdks = fs_enumerate_entries_in_directory(sdk_install_path, FS_ENTRY_DIRECTORY, allocator, temp_allocator);
	return true;
}

int main(int argc, char *argv[]) {
	profile_init(1000 * 1000);

	profile_scope_start("main");

	Arena arena = {};
	arena.capacity = align_to_page_size(512 * 8 * 4096);

	Arena diagnostics_arena = {};
	diagnostics_arena.capacity = align_to_page_size(512 * 8 * 4096);

	Arena temp_arena = {};
	temp_arena.capacity = align_to_page_size(512 * 4096);

	String install_path = {};
	if (!win_sdk_get_install_path(&arena, &install_path)) {
		fprintf(stderr, "Failed to read Windows SDK install path");
		return EXIT_FAILURE;
	}

	install_path = path_append(install_path, STR_LIT("Include"), &arena);

	StringArray sdks = {};
	if (!enum_installed_win_sdks(install_path, &arena, &temp_arena, &sdks)) {
		fprintf(stderr, "Failed to detect Windows SDK");
		return EXIT_FAILURE;
	}

	if (argc >= 2) {
		SourceStorage source_storage = {};

		bool include_win_sdk = true;

		StringArray include_dirs = {};
		include_dirs.values = arena_alloc_array(&arena, String, 0);

		for (size_t i = 2; i < (size_t)argc; i += 1) {
			String arg = str_from_cstr(argv[i]);
			if (arg.length >= 2 && arg.v[0] == '-' && arg.v[1] == 'I') {
				String include_path = sub_str(arg, 2, arg.length - 2);
				assert(include_path.length > 0);
				str_array_append(&include_dirs, &arena, include_path);
			} else if (str_equal(arg, STR_LIT("--no-win-sdk"))) {
				include_win_sdk = false;
			} else {
				fprintf(stderr, "Unknown argument '%s'", argv[i]);
				return EXIT_FAILURE;
			}
		}

		if (include_win_sdk) {
			String sdk_path = path_append(install_path, sdks.values[0], &arena);
			String um_include_path = path_append(sdk_path, STR_LIT("um"), &arena);
			String ucrt_include_path = path_append(sdk_path, STR_LIT("ucrt"), &arena);
			String shared_include_path = path_append(sdk_path, STR_LIT("shared"), &arena);

			str_array_append(&include_dirs, &arena, um_include_path);
			str_array_append(&include_dirs, &arena, ucrt_include_path);
			str_array_append(&include_dirs, &arena, shared_include_path);
		}

		source_storage_init(&source_storage,
				include_dirs,
				&arena);

		SourceFile* source_file = source_storage_append_from_path(&source_storage, str_from_cstr(argv[1]), &temp_arena);

		Diagnostics diagnostics = (Diagnostics) {
			.allocator = &diagnostics_arena,
		};

		Arena generated_tokens_arena = {};
		generated_tokens_arena.capacity = 128 * 4096;

		Preprocessor preprocessor = {};
		preprocessor_init(&preprocessor,
				&source_storage,
				source_file,
				&diagnostics,
				&arena,
				&temp_arena,
				&generated_tokens_arena);

		Arena ident_arena = { .capacity = 128 * 4096 };
		Arena ast_arena = { .capacity = 512 * 4096 };

		IdentifierStorage ident_storage = {};
		ident_storage_init(&ident_storage, heap_allocator_new(), &ident_arena);

		Parser parser = {};
		parser_init(&parser, &ast_arena, &temp_arena, &ident_storage, &preprocessor, &diagnostics);

		ParsedAST parsed_ast = {};
		{
			profile_scope_start("parse");
			parser_parse(&parser, &parsed_ast);
			profile_scope_end();
		}

		if (parsed_ast.root_nodes.first) {
			print_parsed_node(parsed_ast.root_nodes.first);
		}

		if (diagnostics.first == NULL) {
			for (const ParsedNode* node = parsed_ast.root_nodes.first; node != NULL; node = node->next) {
				if (node->kind == AST_NODE_FUNCTION) {
					if (node->function_def->body == NULL) {
						continue;
					}

					Arena input_instr_array_allocator = arena_alloc_sub_arena(&arena, 4096);

					FunctionCompiler c = {};
					c.function = node->function_def;
					c.allocator = &arena;
					c.instr_allocator = &arena;
					c.input_instr_array_allocator = &input_instr_array_allocator;
					c.temp_allocator = &temp_arena;
					c.pointer_type_layout = type_layout_new(8, 8);

					CompiledFunction func = function_compiler_compile(&c);

					instr_replace_dead_instr(func.instr_buffer, func.usage_ranges);
					instr_print_all(func.instr_buffer, &temp_arena);

					InstrIndexArray region_bfs_order = _x64_gather_regions_in_bfs_order(func.instr_buffer,
							&arena,
							&temp_arena,
							func.start_region);

					printf("Region BFS traversal order:\n");
					for (size_t i = 0; i < region_bfs_order.count; i += 1) {
						printf("%u ", region_bfs_order.instr[i].value);
					}
					printf("\n");

					uint16_t allowed_registers = UINT16_MAX;
					allowed_registers &= ~(1 << REG_SP);
					allowed_registers &= ~(1 << REG_BP);

					uint16_t cdecl_arg_regs[] = { REG_A, REG_C, REG_D, REG_8, REG_9 };
					for (size_t i = 0; i < array_size(cdecl_arg_regs); i += 1) {
						allowed_registers &= ~(1 << cdecl_arg_regs[i]);
					}

					X64CodeGenerator gen = {};
					gen.instr_buffer = func.instr_buffer;
					gen.usage_ranges = func.usage_ranges;
					gen.allocator = &arena;
					gen.temp_allocator = &temp_arena;

					x64_alloc_registers(&gen, allowed_registers);
					MachineCodeBuffer machine_code = x64_generate_code(&gen, func.start_region);

					typedef uint64_t(*ExecutableFunction)(int argc, char* argv[]);
					ExecutableFunction executable_function = (ExecutableFunction)machine_code.code;

					uint64_t result = executable_function(argc, argv);

					free_executable(machine_code.code, machine_code.size_in_bytes);

					printf("%llu\n", result);
				}
			}
		}

		ident_storage_release(&ident_storage);

		arena_release(&ident_arena);
		arena_release(&ast_arena);
		arena_release(&generated_tokens_arena);

		diagnostics_print(&diagnostics);
	} else {
		printf("Usage: c <path_to_source_file>\n");
	}

	arena_release(&arena);
	arena_release(&diagnostics_arena);
	arena_release(&temp_arena);

	profile_scope_end();

	profile_finish();

	return EXIT_SUCCESS;
}
