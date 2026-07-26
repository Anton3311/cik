#include <stdio.h>
#include <stdlib.h>
#include <threads.h>

#include "core/core.h"
#include "parser/preprocessor.h"
#include "parser/parser.h"
#include "compiler/compiler.h"
#include "code_gen/backends/x64.h"
#include "code_gen/backends/x64_linker.h"

static bool enum_installed_win_sdks(String sdk_install_path,
		Arena* allocator,
		Arena* temp_allocator,
		StringArray* out_sdks) {

	*out_sdks = fs_enumerate_entries_in_directory(sdk_install_path, FS_ENTRY_DIRECTORY, allocator, temp_allocator);
	return true;
}

typedef enum {
	C_FLAG_NONE                    = 0,
	C_FLAG_KEEP_DEAD_INSTR         = 1 << 0,
	C_FLAG_DO_NOT_INCLUDE_WIN_SDK  = 1 << 1,
	C_FLAG_PRINT_AST               = 1 << 2,
} CompilerFlags;

static const char* s_help_menu = 
	"\n"
	"  Usage:\n"
	"    c.exe <path-to-c-file>\n"
	"\n"
	"  Compiler flags:\n"
	"    --no-win-sdk           don't add Win SDK to include path\n"
	"    -I<include-path>       specify an include path\n"
	"    --show-ast             print AST after parsing\n"
	"\n"
	"  Backend flags:           \n"
	"    --keep-dead-instr      don't eliminate dead instructions\n"
	"    --show-ir              print generated IR instructions\n"
	"    --x64-debug-log        log results of intermediate operations for debugging\n"
	"    --x64-show-instr-loc   print which storage locations were assigned to each instruction";

int main(int argc, char *argv[]) {
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

	CompilerFlags flags = C_FLAG_NONE;
	X64BackendFlags backend_flags = X64_NONE;

	if (argc >= 2) {
		SourceStorage source_storage = {};

		// NOTE: Allocate this before the `include_dirs` array, so that they don't interfere
		String sdk_path = path_append(install_path, sdks.values[0], &arena);
		String um_include_path = path_append(sdk_path, STR_LIT("um"), &arena);
		String ucrt_include_path = path_append(sdk_path, STR_LIT("ucrt"), &arena);
		String shared_include_path = path_append(sdk_path, STR_LIT("shared"), &arena);

		StringArray include_dirs = {};
		include_dirs.values = arena_alloc_array(&arena, String, 0);

		for (size_t i = 2; i < (size_t)argc; i += 1) {
			String arg = str_from_cstr(argv[i]);
			if (arg.length >= 2 && arg.v[0] == '-' && arg.v[1] == 'I') {
				String include_path = sub_str(arg, 2, arg.length - 2);
				
				if (include_path.length == 0) {
					fprintf(stderr, "Include path at argument index %zu is empty", i);
					return EXIT_FAILURE;
				}

				assert(include_path.length > 0);
				str_array_append(&include_dirs, &arena, include_path);
			} else if (str_equal(arg, STR_LIT("--no-win-sdk"))) {
				flags |= C_FLAG_DO_NOT_INCLUDE_WIN_SDK;
			} else if (str_equal(arg, STR_LIT("--keep-dead-instr"))) {
				flags |= C_FLAG_KEEP_DEAD_INSTR;
			} else if (str_equal(arg, STR_LIT("--show-ir"))) {
				backend_flags |= X64_PRINT_SCHEDULED_IR;
			} else if (str_equal(arg, STR_LIT("--show-ast"))) {
				flags |= C_FLAG_PRINT_AST;
			} else if (str_equal(arg, STR_LIT("--x64-debug-log"))) {
				backend_flags |= X64_DEBUG_LOG;
			} else if (str_equal(arg, STR_LIT("--x64-show-instr-loc"))) {
				backend_flags |= X64_PRINT_ASSIGNED_STORAGE_LOC;
			} else if (str_equal(arg, STR_LIT("--"))) {
				break;
			} else {
				fprintf(stderr, "Unknown argument '%s'", argv[i]);
				return EXIT_FAILURE;
			}
		}

		if (!has_flag(flags, C_FLAG_DO_NOT_INCLUDE_WIN_SDK)) {
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
				heap_allocator_new(),
				&arena,
				&temp_arena,
				&generated_tokens_arena);

		Arena ident_arena = { .capacity = 128 * 4096 };
		Arena ast_arena = { .capacity = 512 * 4096 };

		IdentifierStorage ident_storage = {};
		ident_storage_init(&ident_storage, heap_allocator_new(), &ident_arena);

		Parser parser = {};
		parser_init(&parser, &ast_arena, &temp_arena, &ident_storage, &preprocessor, &diagnostics);

		AST parsed_ast = {};
		{
			profile_scope_start("parse");
			parser_parse(&parser, &parsed_ast);
			profile_scope_end();
		}

		preprocessor_release(&preprocessor);

		if (has_flag(flags, C_FLAG_PRINT_AST)) {
			if (parsed_ast.root_nodes.first) {
				print_parsed_node(parsed_ast.root_nodes.first);
			}
		}

		if (diagnostics.first != NULL) {
			diagnostics_print(&diagnostics);
			return EXIT_FAILURE;
		}

		size_t function_count = 0;
		for (const AstNode* node = parsed_ast.root_nodes.first; node != NULL; node = node->next) {
			if (node->kind != AST_NODE_FUNCTION) {
				continue;
			}

			if (node->function_def->body == NULL) {
				continue;
			}

			function_count += 1;
		}

		LoweredFunction* lowered_functions = arena_alloc_array(&arena,
				LoweredFunction,
				function_count);

		FunctionRefTable ref_table = {};
		ref_table.allocator = heap_allocator_new();

		StringStorage string_storage = {};
		string_storage.allocator = heap_allocator_new();

		size_t function_index = 0;
		for (const AstNode* node = parsed_ast.root_nodes.first; node != NULL; node = node->next) {
			if (node->kind != AST_NODE_FUNCTION) {
				continue;
			}

			if (node->function_def->body == NULL) {
				continue;
			}

			uint16_t ref_index = func_ref_table_insert(&ref_table, node->function_def->proto.name);
			ref_table.refs[ref_index].impl_kind = FUNCTION_IMPL_INTERNAL;
			ref_table.refs[ref_index].internal_function_index = function_index;

			FunctionCompiler c = {};
			c.function = node->function_def;
			c.allocator = &arena;
			c.instr_allocator = &arena;
			c.temp_allocator = &temp_arena;
			c.str_storage = &string_storage;
			c.func_ref_table = &ref_table;
			c.pointer_type_layout = type_layout_new(8, 8);

			CompiledFunction func = function_compiler_compile(&c);

			X64CodeGenerator gen = {};
			gen.flags = backend_flags;
			gen.instr_buffer = func.instr_buffer;
			gen.allocator = &arena;
			gen.temp_allocator = &temp_arena;
			gen.ref_table = &ref_table;
			gen.string_consts = str_storage_to_array(c.str_storage);

			lowered_functions[function_index] = x64_generate_code(&gen, func.start_region);

			instr_buffer_release(&gen.instr_buffer);
			function_index += 1;
		}

		compiler_resolve_default_func_refs(&ref_table);

		uint16_t entry_point_id = func_ref_table_entry_index(&ref_table, STR_LIT("main"));
		const FunctionRef* entry_point_ref = &ref_table.refs[entry_point_id];

		assert(entry_point_ref->impl_kind == FUNCTION_IMPL_INTERNAL);

		LinkedProgram linked = linker_link(lowered_functions, function_count, &ref_table, &temp_arena);
		MachineCodeBuffer machine_code = linked.machine_code;

		size_t entry_point_offset = linked.function_offsets[entry_point_ref->internal_function_index];
		void* entry_point_address = (uint8_t*)machine_code.code + entry_point_offset;

		typedef uint64_t(*ExecutableFunction)(int argc, char* argv[]);
		ExecutableFunction entry_point = (ExecutableFunction)entry_point_address;

		int32_t arg_start = argc;
		for (int32_t i = 0; i < argc; i += 1) {
			if (strcmp(argv[i], "--") == 0) {
				arg_start = i + 1;
				break;
			}
		}

		uint64_t result = entry_point(argc - arg_start, argv + arg_start);

		free_executable(machine_code.code, machine_code.size_in_bytes);

		printf("%llu\n", result);

		// Free function symbol table
		func_ref_table_release(&ref_table);
		// Free string storage
		str_storage_release(&string_storage);

		ident_storage_release(&ident_storage);

		arena_release(&ident_arena);
		arena_release(&ast_arena);
		arena_release(&generated_tokens_arena);
	} else {
		printf("No input file\n");
		printf("%s", s_help_menu);
		return EXIT_FAILURE;
	}

	arena_release(&arena);
	arena_release(&diagnostics_arena);
	arena_release(&temp_arena);

#ifdef FEATURE_PROFILER
	// Wait until the profiler connects to upload all the measurements
	profiler_wait_for_connection();
#endif

	return EXIT_SUCCESS;
}
