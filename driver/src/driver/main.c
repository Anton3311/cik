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
	C_FLAG_DO_NOT_INCLUDE_WIN_SDK  = 1 << 0,
	C_FLAG_PRINT_AST               = 1 << 1,
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
	"    --show-ir              print generated IR instructions\n"
	"    --x64-debug-log        log results of intermediate operations for debugging\n"
	"    --x64-show-instr-loc   print which storage locations were assigned to each instruction";

typedef struct {
	Arena* arena;
	Arena* temp_arena;
	Arena* ident_arena;
	Arena* ast_arena;
	Arena* generated_tokens_arena;

	size_t unit_index;

	CompilerFlags flags;
	X64BackendFlags backend_flags;

	String source_file_path;

	Diagnostics* diagnostics;
	SourceStorage* source_storage;

	FunctionRefTable* ref_table;
	SymbolMap* imported_symbol_map;
	SymbolMap* exported_symbol_map;
} CompilationUnitContext;

static LoweredUnit compile_unit(CompilationUnitContext* context) {
	profile_scope_start(__func__);

	SourceFile* source_file = source_storage_append_from_path(
			context->source_storage,
			context->source_file_path,
			context->temp_arena);

	Preprocessor preprocessor = {};
	preprocessor_init(&preprocessor,
			context->source_storage,
			source_file,
			context->diagnostics,
			heap_allocator_new(),
			context->arena,
			context->temp_arena,
			context->generated_tokens_arena);

	IdentifierStorage ident_storage = {};
	ident_storage_init(&ident_storage, heap_allocator_new(), context->ident_arena);

	Parser parser = {};
	parser_init(&parser,
			context->ast_arena,
			context->temp_arena,
			&ident_storage,
			&preprocessor,
			context->diagnostics);

	AST parsed_ast = {};

	{
		profile_scope_start("parse");
		parser_parse(&parser, &parsed_ast);
		profile_scope_end();
	}

	preprocessor_release(&preprocessor);

	if (context->diagnostics->first != NULL) {
		diagnostics_print(context->diagnostics);
		profile_scope_end();
		return (LoweredUnit) {};
	}

	if (has_flag(context->flags, C_FLAG_PRINT_AST) && parsed_ast.root_nodes.first) {
		print_parsed_node(parsed_ast.root_nodes.first);
	}

	uint32_t function_count = 0;
	for (const AstNode* node = parsed_ast.root_nodes.first; node != NULL; node = node->next) {
		if (node->kind != AST_NODE_FUNCTION) {
			continue;
		}

		if (node->function_def->body == NULL) {
			continue;
		}

		function_count += 1;
	}

	LoweredFunction* lowered_functions = arena_alloc_array(context->arena,
			LoweredFunction,
			function_count);

	StringStorage string_storage = {};
	string_storage.allocator = heap_allocator_new();

	FunctionRefTable* ref_table = context->ref_table;

	uint32_t function_index = 0;
	for (const AstNode* node = parsed_ast.root_nodes.first; node != NULL; node = node->next) {
		if (node->kind != AST_NODE_FUNCTION) {
			continue;
		}

		Function* func = node->function_def;

		if (node->function_def->body == NULL) {
			continue;
		}

		{
			Symbol symbol;
			memset(&symbol, 0xff, sizeof(symbol));

			symbol.name = func->proto.name;
			symbol.linkage = func->storage_specifier == STORAGE_SPEC_STATIC
				? SYMBOL_LINKAGE_INTERNAL
				: SYMBOL_LINKAGE_EXTERNAL_STATIC;
			symbol.data.func_index = function_index;

			if (symbol.linkage == SYMBOL_LINKAGE_INTERNAL) {
				symbol.linkage_data.internal.compilation_unit_index = context->unit_index;
			}

			SymbolId symbol_id = symbol_map_insert(context->exported_symbol_map, &symbol);
			assert_msg(symbol_id != SYMBOL_ID_INVALID,
					"Duplicate function symbol. Did the parser miss the redefinition?");
		}

		uint16_t ref_index = func_ref_table_get_or_insert(ref_table, node->function_def->proto.name);
		ref_table->refs[ref_index].impl_kind = FUNCTION_IMPL_INTERNAL;
		ref_table->refs[ref_index].internal.function_index = function_index;
		ref_table->refs[ref_index].internal.compilation_unit_index = context->unit_index;

		FunctionCompiler c = {};
		c.function = node->function_def;
		c.allocator = context->arena;
		c.instr_allocator = context->arena;
		c.temp_allocator = context->temp_arena;
		c.str_storage = &string_storage;
		c.func_ref_table = ref_table;
		c.symbol_map = context->imported_symbol_map;
		c.pointer_type_layout = type_layout_new(8, 8);

		CompiledFunction compiled_function = function_compiler_compile(&c);

		X64CodeGenerator gen = {};
		gen.flags = context->backend_flags;
		gen.instr_buffer = compiled_function.instr_buffer;
		gen.allocator = context->arena;
		gen.temp_allocator = context->temp_arena;
		gen.ref_table = ref_table;
		gen.string_consts = str_storage_to_array(c.str_storage);

		lowered_functions[function_index] = x64_generate_code(&gen, compiled_function.start_region);

		instr_buffer_release(&gen.instr_buffer);
		function_index += 1;
	}

	// Free string storage
	str_storage_release(&string_storage);

	ident_storage_release(&ident_storage);

	profile_scope_end();
	return (LoweredUnit) {
		.functions = lowered_functions,
		.function_count = function_count,
	};
}

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
		str_array_reserve(&include_dirs, &arena, argc + 3);
		
		StringArray source_files = {};
		str_array_reserve(&source_files, &arena, argc);

		for (size_t i = 1; i < (size_t)argc; i += 1) {
			String arg = str_from_cstr(argv[i]);
			if (arg.length >= 2 && arg.v[0] == '-' && arg.v[1] == 'I') {
				String include_path = sub_str(arg, 2, arg.length - 2);
				
				if (include_path.length == 0) {
					fprintf(stderr, "Include path at argument index %zu is empty", i);
					return EXIT_FAILURE;
				}

				assert(include_path.length > 0);

				str_array_append_assume_cap(&include_dirs, include_path);
			} else if (str_ends_with(arg, STR_LIT(".c"))) {
				str_array_append_assume_cap(&source_files, arg);
			} else if (str_equal(arg, STR_LIT("--no-win-sdk"))) {
				flags |= C_FLAG_DO_NOT_INCLUDE_WIN_SDK;
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
			str_array_append_assume_cap(&include_dirs, um_include_path);
			str_array_append_assume_cap(&include_dirs, ucrt_include_path);
			str_array_append_assume_cap(&include_dirs, shared_include_path);
		}

		source_storage_init(&source_storage,
				include_dirs,
				&arena);

		FunctionRefTable ref_table = {};
		ref_table.allocator = heap_allocator_new();

		Arena generated_tokens_arena = { .capacity = 128 * 4096 };
		Arena ident_arena = { .capacity = 128 * 4096 };
		Arena ast_arena = { .capacity = 512 * 4096 };

		SymbolMap* imported_symbol_maps = arena_alloc_array(&arena, SymbolMap, source_files.count);
		SymbolMap* exported_symbol_maps = arena_alloc_array(&arena, SymbolMap, source_files.count);
		LoweredUnit* lowered_units = arena_alloc_array(&arena, LoweredUnit, source_files.count);

		for (size_t i = 0; i < source_files.count; i += 1) {
			symbol_map_init(&imported_symbol_maps[i], heap_allocator_new());
		}

		for (size_t i = 0; i < source_files.count; i += 1) {
			symbol_map_init(&exported_symbol_maps[i], heap_allocator_new());
		}

		SymbolMap dynamically_linked_symbols = {};
		symbol_map_init(&dynamically_linked_symbols, heap_allocator_new());

		compiler_resolve_default_func_refs(&dynamically_linked_symbols);

		for (size_t i = 0; i < source_files.count; i += 1) {
			Diagnostics diagnostics = (Diagnostics) {
				.allocator = &diagnostics_arena,
			};

			CompilationUnitContext context = {};
			context.unit_index = i;
			context.flags = flags;
			context.backend_flags = backend_flags;
			context.arena = &arena;
			context.temp_arena = &temp_arena;
			context.ident_arena = &ident_arena;
			context.ast_arena = &ast_arena;
			context.generated_tokens_arena = &generated_tokens_arena;
			context.source_file_path = source_files.values[i];
			context.ref_table = &ref_table;
			context.exported_symbol_map = &exported_symbol_maps[i];
			context.imported_symbol_map = &imported_symbol_maps[i];
			context.diagnostics = &diagnostics;
			context.source_storage = &source_storage;

		 	lowered_units[i] = compile_unit(&context);
		}

		LinkedProgram linked = linker_link(lowered_units,
				imported_symbol_maps,
				exported_symbol_maps,
				&dynamically_linked_symbols,
				source_files.count,
				STR_LIT("main"),
				&arena);
		MachineCodeBuffer machine_code = linked.machine_code;

		assert(linked.entry_point_address);

		typedef uint64_t(*ExecutableFunction)(int argc, char* argv[]);
		ExecutableFunction entry_point = (ExecutableFunction)linked.entry_point_address;

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

		symbol_map_release(&dynamically_linked_symbols);

		for (size_t i = 0; i < source_files.count; i += 1) {
			symbol_map_release(&imported_symbol_maps[i]);
		}

		for (size_t i = 0; i < source_files.count; i += 1) {
			symbol_map_release(&exported_symbol_maps[i]);
		}

		func_ref_table_release(&ref_table);

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
