#include "tests_x64.h"

#include "compiler/compiler.h"
#include "parser/preprocessor.h"
#include "parser/parser.h"
#include "code_gen/backends/x64.h"
#include "code_gen/backends/x64_linker.h"

#define DEFAULT_SOURCE_FILE_PATH "test.c"

typedef uint64_t(*ExecutableFunction)();
typedef void(*ResolverFunction)(SymbolMap* map, void* data);

static MachineCodeBuffer _compile_with_custom_symbols(TestContext* context,
		String source_code,
		ResolverFunction resolver,
		void* resolver_data) {

	SourceStorage source_storage = {};

	StringArray include_dirs = {};
	source_storage_init(&source_storage,
			include_dirs,
			context->arena);

	SourceFile* source_file = source_storage_append(
			&source_storage,
			STR_LIT(DEFAULT_SOURCE_FILE_PATH),
			source_code);

	Diagnostics diagnostics = (Diagnostics) {
		.allocator = context->arena,
		.source_storage = &source_storage,
		.error_limit = 128,
	};

	Arena generated_tokens_arena = {};
	generated_tokens_arena.capacity = 128 * 4096;

	Preprocessor preprocessor = {};
	preprocessor_init(&preprocessor,
			&source_storage,
			source_file,
			&diagnostics,
			heap_allocator_new(),
			context->arena,
			context->temp_arena,
			&generated_tokens_arena);

	Arena ident_arena = { .capacity = 128 * 4096 };
	Arena ast_arena = { .capacity = 512 * 4096 };

	IdentifierStorage ident_storage = {};
	ident_storage_init(&ident_storage, heap_allocator_new(), &ident_arena);

	TypeContext type_context = {
		.pointer_type_layout = {
			.size = sizeof(void*),
			.alignment = alignof(void*)
		}
	};

	Parser parser = {};
	parser_init(&parser,
			&ast_arena,
			context->arena,
			&ident_storage,
			&type_context,
			&preprocessor,
			&diagnostics);

	AST parsed_ast = {};
	parser_parse(&parser, &parsed_ast);

	preprocessor_release(&preprocessor);

	if (diagnostics.first) {
		diagnostics_print(&diagnostics);
		panic("Failed to parse");
	}

	size_t function_count = 0;
	for (const AstNode* node = parsed_ast.root_nodes.first; node != NULL; node = node->next) {
		if (node->kind != AST_NODE_FUNCTION_DEF) {
			continue;
		}

		if (node->function_def->body == NULL) {
			continue;
		}

		function_count += 1;
	}

	assert_msg(function_count == 1, "Multiple functions are not supported in tests");

	size_t unit_count = 1;
	LoweredFunction* lowered_functions = arena_alloc_array(context->arena,
			LoweredFunction,
			function_count);

	SymbolMap dynamically_linked_symbols = {};
	symbol_map_init(&dynamically_linked_symbols, heap_allocator_new());
	compiler_resolve_default_func_refs(&dynamically_linked_symbols);

	if (resolver) {
		resolver(&dynamically_linked_symbols, resolver_data);
	}

	SymbolMap imported_symbol_map = {};
	symbol_map_init(&imported_symbol_map, heap_allocator_new());

	SymbolMap exported_symbol_map = {};
	symbol_map_init(&exported_symbol_map, heap_allocator_new());

	Arena strings_arena = arena_alloc_sub_arena(context->arena, 1024);

	StringStorage string_storage = {};
	string_storage.allocator = arena_allocator_new(&strings_arena);

	compiler_collect_imported_symbols(&parsed_ast, &imported_symbol_map);

	size_t function_index = 0;
	for (const AstNode* node = parsed_ast.root_nodes.first; node != NULL; node = node->next) {
		if (node->kind != AST_NODE_FUNCTION_DEF) {
			continue;
		}

		if (node->function_def->body == NULL) {
			continue;
		}

		Function* func = node->function_def;

		{
			Symbol symbol;
			memset(&symbol, 0xff, sizeof(symbol));

			symbol.name = func->proto.name;
			symbol.linkage = func->storage_specifier == STORAGE_SPEC_STATIC
				? SYMBOL_LINKAGE_INTERNAL
				: SYMBOL_LINKAGE_EXTERNAL_STATIC;
			symbol.data.func_index = function_index;

			if (symbol.linkage == SYMBOL_LINKAGE_INTERNAL) {
				symbol.linkage_data.internal.compilation_unit_index = 0;
			}

			SymbolId symbol_id = symbol_map_insert(&exported_symbol_map, &symbol);
			assert_msg(symbol_id != SYMBOL_ID_INVALID,
					"Duplicate function symbol. Did the parser miss the redefinition?");
		}

		FunctionCompiler c = {};
		c.function = node->function_def;
		c.allocator = context->arena;
		c.instr_allocator = context->arena;
		c.temp_allocator = context->temp_arena;
		c.symbol_map = &imported_symbol_map;
		c.str_storage = &string_storage;
		c.type_context = &type_context;

		CompiledFunction compiled_function = function_compiler_compile(&c);

		X64CodeGenerator gen = {};
		gen.instr_buffer = compiled_function.instr_buffer;
		gen.allocator = context->arena;
		gen.temp_allocator = context->temp_arena;
		gen.string_consts = str_storage_to_array(&string_storage);
		gen.flags = X64_PRINT_SCHEDULED_IR;
		gen.function_signature = function_prototype_to_abi_signature(&type_context,
				&node->function_def->proto,
				arena_allocator_new(context->temp_arena));
		gen.function_call_signatures = compiled_function.function_call_signatures;

		lowered_functions[function_index] = x64_generate_code(&gen, compiled_function.start_region);

		instr_buffer_release(&gen.instr_buffer);
		function_index += 1;
	}

	LoweredUnit unit = {};
	unit.functions = lowered_functions;
	unit.function_count = function_count;

	LinkedProgram linked;
	bool link_successful = linker_link(&unit,
			&imported_symbol_map,
			&exported_symbol_map,
			&dynamically_linked_symbols,
			1,
			STR_LIT("main"),
			context->arena,
			context->temp_arena,
			&linked);

	if (!link_successful) {
		linker_print_errors(linked.errors,
				linked.error_count,
				&imported_symbol_map,
				&exported_symbol_map);
	}

	assert(link_successful);

	symbol_map_release(&dynamically_linked_symbols);
	symbol_map_release(&imported_symbol_map);
	symbol_map_release(&exported_symbol_map);

	assert(linked.entry_point_address == linked.machine_code.code);
	return linked.machine_code;
}

static MachineCodeBuffer _compile(TestContext* context, String source_code) {
	return _compile_with_custom_symbols(context, source_code, NULL, NULL);
}

void test_return_uint64_zero(TestContext* context) {
	String source_code = STR_LIT("unsigned long long main() { return 0; }");
	MachineCodeBuffer machine_code = _compile(context, source_code);

	ExecutableFunction executable_function = (ExecutableFunction)machine_code.code;
	uint64_t result = executable_function();

	assert(result == 0);
	free_executable(machine_code.code, machine_code.size_in_bytes);
}

void test_add_uint64_consts(TestContext* context) {
	String source_code = STR_LIT("unsigned long long main() { return 10 + 15; }");
	MachineCodeBuffer machine_code = _compile(context, source_code);

	ExecutableFunction executable_function = (ExecutableFunction)machine_code.code;
	uint64_t result = executable_function();

	assert(result == 25);
	free_executable(machine_code.code, machine_code.size_in_bytes);
}

void test_return_first_arg(TestContext* context) {
	String source_code = STR_LIT("unsigned long long main(unsigned long long first) { return first; }");
	MachineCodeBuffer machine_code = _compile(context, source_code);

	typedef uint64_t(*Function)(uint64_t first);

	uint64_t input = rand();

	Function executable_function = (Function)machine_code.code;
	uint64_t result = executable_function(input);

	assert(result == input);
	free_executable(machine_code.code, machine_code.size_in_bytes);
}

void test_return_sum_of_first_two_args(TestContext* context) {
	String source_code = STR_LIT(
			"typedef unsigned long long uint64;\n"
			"uint64 main(uint64 a, uint64 b) { return a + b; }");
	MachineCodeBuffer machine_code = _compile(context, source_code);

	typedef uint64_t(*Function)(uint64_t a, uint64_t b);

	uint64_t a = rand();
	uint64_t b = rand();

	Function executable_function = (Function)machine_code.code;
	uint64_t result = executable_function(a, b);

	assert(result == a + b);
	free_executable(machine_code.code, machine_code.size_in_bytes);
}

void test_deref_function_arg(TestContext* context) {
	String source_code = STR_LIT(
			"typedef unsigned long long uint64;\n"
			"uint64 main(uint64* ptr) { return *ptr; }");
	MachineCodeBuffer machine_code = _compile(context, source_code);

	typedef uint64_t(*Function)(uint64_t* ptr);

	uint64_t value = rand();

	Function executable_function = (Function)machine_code.code;
	uint64_t result = executable_function(&value);

	assert(result == value);
	free_executable(machine_code.code, machine_code.size_in_bytes);
}

void test_index_arary_with_pointer_arithmetics(TestContext* context) {
	String source_code = STR_LIT(
			"typedef unsigned long long uint64;\n"
			"uint64 main(uint64* ptr, uint64 index) { return *(ptr + index); }");
	MachineCodeBuffer machine_code = _compile(context, source_code);

	typedef uint64_t(*Function)(uint64_t* ptr, uint64_t index);

	uint64_t array[16];
	for (size_t i = 0; i < array_size(array); i += 1) {
		array[i] = rand();
	}

	Function executable_function = (Function)machine_code.code;

	uint64_t results[16];
	for (size_t i = 0; i < array_size(array); i += 1) {
		results[i] = executable_function(array, (uint64_t)i);
	}

	for (size_t i = 0; i < array_size(array); i += 1) {
		assert(results[i] == array[i]);
	}

	free_executable(machine_code.code, machine_code.size_in_bytes);
}

void test_index_arary_with_pointer_arithmetics_2(TestContext* context) {
	String source_code = STR_LIT(
			"typedef unsigned long long uint64;\n"
			"uint64 main(uint64* ptr, uint64 index) { return *(index + ptr); }");
	MachineCodeBuffer machine_code = _compile(context, source_code);

	typedef uint64_t(*Function)(uint64_t* ptr, uint64_t index);

	uint64_t array[16];
	for (size_t i = 0; i < array_size(array); i += 1) {
		array[i] = rand();
	}

	Function executable_function = (Function)machine_code.code;

	uint64_t results[16];
	for (size_t i = 0; i < array_size(array); i += 1) {
		results[i] = executable_function(array, (uint64_t)i);
	}

	for (size_t i = 0; i < array_size(array); i += 1) {
		assert(results[i] == array[i]);
	}

	free_executable(machine_code.code, machine_code.size_in_bytes);
}

void test_compare_equal_two_uint64(TestContext* context) {
	String source_code = STR_LIT(
			"typedef unsigned long long uint64;\n"
			"uint64 main(uint64 a, uint64 b) { return a == b; }");
	MachineCodeBuffer machine_code = _compile(context, source_code);

	typedef uint64_t(*Function)(uint64_t a, uint64_t b);

#define SAMPLE_COUNT 16

	uint64_t array_a[SAMPLE_COUNT];
	uint64_t array_b[SAMPLE_COUNT];
	for (size_t i = 0; i < SAMPLE_COUNT; i += 1) {
		array_a[i] = rand();
		array_b[i] = rand();
	}

	Function executable_function = (Function)machine_code.code;

	uint64_t results[SAMPLE_COUNT];
	for (size_t i = 0; i < SAMPLE_COUNT; i += 1) {
		results[i] = executable_function(array_a[i], array_b[i]);
	}

	for (size_t i = 0; i < SAMPLE_COUNT; i += 1) {
		assert(results[i] == (array_a[i] == array_b[i]));
	}

	free_executable(machine_code.code, machine_code.size_in_bytes);
}

void test_compare_equal_less_for_uint64(TestContext* context) {
	String source_code = STR_LIT(
			"typedef unsigned long long uint64;\n"
			"uint64 main(uint64 a, uint64 b) { return a < b; }");
	MachineCodeBuffer machine_code = _compile(context, source_code);

	typedef uint64_t(*Function)(uint64_t a, uint64_t b);

#define SAMPLE_COUNT 16

	uint64_t array_a[SAMPLE_COUNT];
	uint64_t array_b[SAMPLE_COUNT];
	for (size_t i = 0; i < SAMPLE_COUNT; i += 1) {
		array_a[i] = rand();
		array_b[i] = rand();
	}

	Function executable_function = (Function)machine_code.code;

	uint64_t results[SAMPLE_COUNT];
	for (size_t i = 0; i < SAMPLE_COUNT; i += 1) {
		results[i] = executable_function(array_a[i], array_b[i]);
	}

	for (size_t i = 0; i < SAMPLE_COUNT; i += 1) {
		assert(results[i] == (array_a[i] < array_b[i]));
	}

	free_executable(machine_code.code, machine_code.size_in_bytes);
}

void test_compare_equal_greater_for_uint64(TestContext* context) {
	String source_code = STR_LIT(
			"typedef unsigned long long uint64;\n"
			"uint64 main(uint64 a, uint64 b) { return a > b; }");
	MachineCodeBuffer machine_code = _compile(context, source_code);

	typedef uint64_t(*Function)(uint64_t a, uint64_t b);

#define SAMPLE_COUNT 16

	uint64_t array_a[SAMPLE_COUNT];
	uint64_t array_b[SAMPLE_COUNT];
	for (size_t i = 0; i < SAMPLE_COUNT; i += 1) {
		array_a[i] = rand();
		array_b[i] = rand();
	}

	Function executable_function = (Function)machine_code.code;

	uint64_t results[SAMPLE_COUNT];
	for (size_t i = 0; i < SAMPLE_COUNT; i += 1) {
		results[i] = executable_function(array_a[i], array_b[i]);
	}

	for (size_t i = 0; i < SAMPLE_COUNT; i += 1) {
		assert(results[i] == (array_a[i] > array_b[i]));
	}

	free_executable(machine_code.code, machine_code.size_in_bytes);
}

void test_mutate_argument(TestContext* context) {
	String source_code = STR_LIT(
			"typedef unsigned long long uint64;\n"
			"uint64 main(uint64 a) { a = 100; return a; }");
	MachineCodeBuffer machine_code = _compile(context, source_code);

	typedef uint64_t(*Function)(uint64_t a);

	Function executable_function = (Function)machine_code.code;

	uint64_t result = executable_function(101);

	assert(result == 100);
	free_executable(machine_code.code, machine_code.size_in_bytes);
}

static void _internal_store_1(uint64_t* out) {
	*out = 1;
}

static void _internal_store_2(uint64_t* out) {
	*out = 2;
}

static void _internal_store_3(uint64_t* out) {
	*out = 3;
}

static void _internal_store_4(uint64_t* out) {
	*out = 4;
}

static void _resolve_symbols_for_call_inside_inner_scope(SymbolMap* map, void* data) {
	symbol_map_insert_dynamically_linked_impl(map, STR_LIT("store_1"), _internal_store_1);
	symbol_map_insert_dynamically_linked_impl(map, STR_LIT("store_2"), _internal_store_2);
	symbol_map_insert_dynamically_linked_impl(map, STR_LIT("store_3"), _internal_store_3);
	symbol_map_insert_dynamically_linked_impl(map, STR_LIT("store_4"), _internal_store_4);
}

void test_call_inside_inner_scope(TestContext* context) {
	String source_code = STR_LIT(
			"typedef unsigned long long uint64_t;\n"
			"__declspec(dllimport) void store_1(uint64_t* out);\n"
			"__declspec(dllimport) void store_2(uint64_t* out);\n"
			"__declspec(dllimport) void store_3(uint64_t* out);\n"
			"uint64_t main(uint64_t* out) {\n"
			"    store_1(out + 0);\n"
			"    {\n"
			"        store_2(out + 1);\n"
			"    }\n"
			"    store_3(out + 2);\n"
			"    return 0;\n"
			"}\n");
	MachineCodeBuffer machine_code = _compile_with_custom_symbols(context,
			source_code,
			_resolve_symbols_for_call_inside_inner_scope,
			NULL);

	typedef uint64_t(*Function)(uint64_t*);

	Function executable_function = (Function)machine_code.code;

	uint64_t ints[3] = { 0 };
	uint64_t result = executable_function(ints);
	assert(result == 0);
	
	assert(ints[0] == 1);
	assert(ints[1] == 2);
	assert(ints[2] == 3);

	free_executable(machine_code.code, machine_code.size_in_bytes);
}

void test_conditional_call_1(TestContext* context) {
	String source_code = STR_LIT(
			"typedef unsigned long long uint64_t;\n"
			"__declspec(dllimport) void store_1(uint64_t* out);\n"
			"__declspec(dllimport) void store_2(uint64_t* out);\n"
			"__declspec(dllimport) void store_3(uint64_t* out);\n"
			"uint64_t main(uint64_t cond, uint64_t* out) {\n"
			"    if (cond == 1) {\n"
			"        store_1(out);\n"
			"    } else {\n"
			"    }"
			"    return 0;\n"
			"}\n");
	MachineCodeBuffer machine_code = _compile_with_custom_symbols(context,
			source_code,
			_resolve_symbols_for_call_inside_inner_scope,
			NULL);

	typedef uint64_t(*Function)(uint64_t, uint64_t*);

	Function executable_function = (Function)machine_code.code;

	uint64_t result = 0;
	uint64_t exit_code = executable_function(1, &result);
	assert(exit_code == 0);
	assert(result == 1);

	free_executable(machine_code.code, machine_code.size_in_bytes);
}

void test_conditional_call_2(TestContext* context) {
	String source_code = STR_LIT(
			"typedef unsigned long long uint64_t;\n"
			"__declspec(dllimport) void store_1(uint64_t* out);\n"
			"__declspec(dllimport) void store_2(uint64_t* out);\n"
			"__declspec(dllimport) void store_3(uint64_t* out);\n"
			"uint64_t main(uint64_t cond, uint64_t* out) {\n"
			"    if (cond == 1) {\n"
			"        store_1(out);\n"
			"    } else {\n"
			"        store_2(out);"
			"    }"
			"    return 0;\n"
			"}\n");
	MachineCodeBuffer machine_code = _compile_with_custom_symbols(context,
			source_code,
			_resolve_symbols_for_call_inside_inner_scope,
			NULL);

	typedef uint64_t(*Function)(uint64_t, uint64_t*);

	Function executable_function = (Function)machine_code.code;

	uint64_t result = 0;
	uint64_t exit_code = executable_function(0, &result);
	assert(exit_code == 0);
	assert(result == 2);

	free_executable(machine_code.code, machine_code.size_in_bytes);
}

static MachineCodeBuffer _compile_conditional_call_between_two_calls(TestContext* context) {
	String source_code = STR_LIT(
			"typedef unsigned long long uint64_t;\n"
			"__declspec(dllimport) void store_1(uint64_t* out);\n"
			"__declspec(dllimport) void store_2(uint64_t* out);\n"
			"__declspec(dllimport) void store_3(uint64_t* out);\n"
			"__declspec(dllimport) void store_4(uint64_t* out);\n"
			"uint64_t main(uint64_t cond, uint64_t* out) {\n"
			"    store_1(out + 0);"
			"    if (cond == 1) {\n"
			"        store_2(out + 1);\n"
			"    } else {\n"
			"        store_3(out + 2);\n"
			"    }\n"
			"    store_4(out + 3);\n"
			"    return 0;\n"
			"}\n");
	MachineCodeBuffer machine_code = _compile_with_custom_symbols(context,
			source_code,
			_resolve_symbols_for_call_inside_inner_scope,
			NULL);

	return machine_code;
}

void test_conditional_call_between_two_calls_1(TestContext* context) {
	MachineCodeBuffer machine_code = _compile_conditional_call_between_two_calls(context);

	typedef uint64_t(*Function)(uint64_t, uint64_t*);
	Function executable_function = (Function)machine_code.code;

	uint64_t ints[4] = { 0 };
	uint64_t result = executable_function(0, ints);
	assert(result == 0);
	
	assert(ints[0] == 1);
	assert(ints[1] == 0);
	assert(ints[2] == 3);
	assert(ints[3] == 4);

	free_executable(machine_code.code, machine_code.size_in_bytes);
}

void test_conditional_call_between_two_calls_2(TestContext* context) {
	MachineCodeBuffer machine_code = _compile_conditional_call_between_two_calls(context);

	typedef uint64_t(*Function)(uint64_t, uint64_t*);
	Function executable_function = (Function)machine_code.code;

	uint64_t ints[4] = { 0 };
	uint64_t result = executable_function(1, ints);
	assert(result == 0);
	
	assert(ints[0] == 1);
	assert(ints[1] == 2);
	assert(ints[2] == 0);
	assert(ints[3] == 4);

	free_executable(machine_code.code, machine_code.size_in_bytes);
}

static MachineCodeBuffer _compile_return_one_phi_node_value(TestContext* context) {
	String source_code = STR_LIT(
			"typedef unsigned long long uint64_t;\n"
			"uint64_t main(uint64_t cond) {\n"
			"    uint64_t result;\n"
			"    if (cond == 1) {\n"
			"        result = 10;\n"
			"    } else {\n"
			"        result = 88;\n"
			"    }\n"
			"    return result;\n"
			"}\n");

	return _compile(context, source_code);
}

void test_return_one_phi_node_1(TestContext* context) {
	MachineCodeBuffer machine_code = _compile_return_one_phi_node_value(context);

	typedef uint64_t(*Function)(uint64_t);
	Function executable_function = (Function)machine_code.code;

	uint64_t result = executable_function(1);
	assert(result == 10);

	free_executable(machine_code.code, machine_code.size_in_bytes);
}

void test_return_one_phi_node_2(TestContext* context) {
	MachineCodeBuffer machine_code = _compile_return_one_phi_node_value(context);

	typedef uint64_t(*Function)(uint64_t);
	Function executable_function = (Function)machine_code.code;

	uint64_t result = executable_function(0);
	assert(result == 88);

	free_executable(machine_code.code, machine_code.size_in_bytes);
}

void test_return_sum_of_phi_node_values(TestContext* context) {
	String source_code = STR_LIT(
			"typedef unsigned long long uint64_t;\n"
			"uint64_t main(uint64_t cond) {\n"
			"    uint64_t result_1;\n"
			"    uint64_t result_2;\n"
			"    if (cond == 1) {\n"
			"        result_1 = 10;\n"
			"        result_2 = 2;\n"
			"    } else {\n"
			"        result_1 = 88;\n"
			"        result_2 = 22;\n"
			"    }\n"
			"    return result_1 + result_2;\n"
			"}\n");

	MachineCodeBuffer machine_code = _compile(context, source_code);

	typedef uint64_t(*Function)(uint64_t);
	Function executable_function = (Function)machine_code.code;

	uint64_t result = executable_function(0);
	assert(result == 110);

	free_executable(machine_code.code, machine_code.size_in_bytes);
}

void test_phi_in_nested_if_else(TestContext* context) {
	String source_code = STR_LIT(
			"typedef unsigned long long uint64_t;\n"
			"uint64_t main(uint64_t primary, uint64_t secondary) {\n"
			"    uint64_t result;\n"
			"    if (primary == 10) {\n"
			"        if (secondary == 99) {\n"
			"            result = 8;\n"
			"        } else {\n"
			"            result = 11;\n"
			"        }\n"
			"    } else {\n"
			"        result = 3;\n"
			"    }\n"
			"    return result;\n"
			"}\n");

	MachineCodeBuffer machine_code = _compile(context, source_code);

	typedef uint64_t(*Function)(uint64_t, uint64_t);
	Function executable_function = (Function)machine_code.code;

	assert(executable_function(10, 0) == 11);
	assert(executable_function(10, 99) == 8);
	assert(executable_function(8, 0) == 3);

	free_executable(machine_code.code, machine_code.size_in_bytes);
}

void test_phi_in_if_without_else(TestContext* context) {
	String source_code = STR_LIT(
			"typedef unsigned long long uint64_t;\n"
			"uint64_t main(uint64_t cond) {\n"
			"    uint64_t result = 10;\n"
			"    if (cond == 10) {\n"
		    "        result = 11;\n"
			"    }\n"
			"    return result;\n"
			"}\n");

	MachineCodeBuffer machine_code = _compile(context, source_code);

	typedef uint64_t(*Function)(uint64_t);
	Function executable_function = (Function)machine_code.code;

	assert(executable_function(10) == 11);
	assert(executable_function(1) == 10);

	free_executable(machine_code.code, machine_code.size_in_bytes);
}

void test_phi_in_nested_if_without_else(TestContext* context) {
	String source_code = STR_LIT(
			"typedef unsigned long long uint64_t;\n"
			"uint64_t main(uint64_t primary, uint64_t secondary) {\n"
			"    uint64_t result = 5;\n"
			"    if (primary == 10) {\n"
			"        if (secondary == 99) {\n"
			"            result = 8;\n"
			"        }\n"
			"    } else {\n"
			"        if (secondary == 0) {\n"
			"            result = 2;\n"
			"        }\n"
			"    }\n"
			"    return result;\n"
			"}\n");

	MachineCodeBuffer machine_code = _compile(context, source_code);

	typedef uint64_t(*Function)(uint64_t, uint64_t);
	Function executable_function = (Function)machine_code.code;

	assert(executable_function(10, 0) == 5);
	assert(executable_function(10, 99) == 8);
	assert(executable_function(8, 4) == 5);
	assert(executable_function(8, 0) == 2);

	free_executable(machine_code.code, machine_code.size_in_bytes);
}

void test_phi_placement_during_conditional_function_arg_assignment(TestContext* context) {
	String source_code = STR_LIT(
			"typedef unsigned int uint32_t;\n"
			"uint32_t main(uint32_t primary, uint32_t secondary) {\n"
			"    if (primary == 10) {\n"
			"        secondary = 8;\n"
			"    } else {\n"
			"        secondary = 4;\n"
			"    }\n"
			"    return secondary;\n"
			"}\n");

	MachineCodeBuffer machine_code = _compile(context, source_code);

	typedef uint32_t(*Function)(uint32_t, uint32_t);
	Function executable_function = (Function)machine_code.code;

	assert(executable_function(10, 0) == 8);
	assert(executable_function(0, 0) == 4);

	free_executable(machine_code.code, machine_code.size_in_bytes);
}

void test_min(TestContext* context) {
	String source_code = STR_LIT(
			"typedef unsigned long long uint64_t;\n"
			"uint64_t main(uint64_t a, uint64_t b) {\n"
			"    uint64_t result;\n"
			"    if (a < b) { result = a; } else { result = b; }\n"
			"    return result;\n"
			"}\n");

	MachineCodeBuffer machine_code = _compile(context, source_code);

	typedef uint64_t(*Function)(uint64_t, uint64_t);
	Function executable_function = (Function)machine_code.code;

	assert(executable_function(10, 9) == 9);
	assert(executable_function(4, 3) == 3);
	assert(executable_function(9812, 7777881) == 9812);
	assert(executable_function(10, 10) == 10);

	free_executable(machine_code.code, machine_code.size_in_bytes);
}

void test_char_to_lower(TestContext* context) {
	String source_code = STR_LIT(
			"char main(unsigned int a) {\n"
			"    char result;\n"
			"    if (a >= 'A') {\n"
			"        if (a <= 'Z') {\n"
			"            result = 'a' + a - 'A';\n"
			"        } else {\n"
			"            result = a;\n"
			"        }\n"
			"    } else {\n"
			"        result = a;\n"
			"    }\n"
			"    return result;\n"
			"}\n");

	MachineCodeBuffer machine_code = _compile(context, source_code);

	typedef char(*Function)(uint32_t);
	Function executable_function = (Function)machine_code.code;

	for (uint32_t i = 0; i < 0xff; i += 1) {
		char input = (char)i;
		char a = executable_function(input);

		assert(a == tolower(input));
	}

	free_executable(machine_code.code, machine_code.size_in_bytes);
}

void test_char_to_upper(TestContext* context) {
	String source_code = STR_LIT(
			"char main(unsigned int a) {\n"
			"    char result;\n"
			"    if (a >= 'a') {\n"
			"        if (a <= 'z') {\n"
			"            result = 'A' + a - 'a';\n"
			"        } else {\n"
			"            result = a;\n"
			"        }\n"
			"    } else {\n"
			"        result = a;\n"
			"    }\n"
			"    return result;\n"
			"}\n");

	MachineCodeBuffer machine_code = _compile(context, source_code);

	typedef char(*Function)(uint32_t);
	Function executable_function = (Function)machine_code.code;

	for (uint32_t i = 0; i < 0xff; i += 1) {
		char input = (char)i;
		char a = executable_function(input);

		assert(a == toupper(input));
	}

	free_executable(machine_code.code, machine_code.size_in_bytes);
}

void test_return_file_path(TestContext* context) {
	String source_code = STR_LIT(
			"const char* main() {\n"
			"	return __FILE__;\n"
			"}\n");

	MachineCodeBuffer machine_code = _compile(context, source_code);

	typedef const char*(*Function)();
	Function executable_function = (Function)machine_code.code;

	const char* file_path = executable_function();
	assert(strcmp(file_path, DEFAULT_SOURCE_FILE_PATH) == 0);

	free_executable(machine_code.code, machine_code.size_in_bytes);
}

static MachineCodeBuffer _lowered_code_to_machine_code(const LoweredFunction* lowered) {
	MachineCodeBuffer code = {};
	code.size_in_bytes = lowered->size_in_bytes;
	code.code = allocate_executable(code.size_in_bytes);

	memcpy(code.code, lowered->code, code.size_in_bytes);
	return code;
}

void test_sub_instr_code_gen_for_different_reg_configurations(TestContext* context) {
	Arena* instr_allocator = context->arena;

	InstrBuffer buffer = {};
	InstrBuffer* instr_buffer = &buffer;
	instr_buffer_init(instr_buffer, instr_allocator);

	// Setup out test program, which just computes 10 - 5 and returns the result.
	InstrIndex left_operand_index = instr_buffer_append(instr_buffer, instr_allocator);
	InstrIndex right_operand_index = instr_buffer_append(instr_buffer, instr_allocator);
	InstrIndex bin_op_index = instr_buffer_append(instr_buffer, instr_allocator);
	InstrIndex io_state_index = instr_buffer_append(instr_buffer, instr_allocator);
	InstrIndex return_index = instr_buffer_append(instr_buffer, instr_allocator);
	InstrIndex region_index = instr_new_region(instr_buffer, instr_allocator);

	{
		Instr* left_operand = instr_buffer_at(instr_buffer, left_operand_index);
		left_operand->kind = INSTR_CONST_32;
		left_operand->const_32.u = 100;
	}

	{
		Instr* right_operand = instr_buffer_at(instr_buffer, right_operand_index);
		right_operand->kind = INSTR_CONST_32;
		right_operand->const_32.u = 4;
	}

	{
		Instr* bin_op = instr_buffer_at(instr_buffer, bin_op_index);
		bin_op->kind = INSTR_BIN_OP_32;
		bin_op->bin_op.kind = INSTR_BIN_SUB;
		bin_op->bin_op.left = left_operand_index;
		bin_op->bin_op.right = right_operand_index;
	}

	{
		Instr* io_state = instr_buffer_at(instr_buffer, io_state_index);
		io_state->kind = INSTR_IO_STATE;
		io_state->io_state.producer = INVALID_INSTR_INDEX;
	}

	{
		Instr* ret = instr_buffer_at(instr_buffer, return_index);
		ret->kind = INSTR_RETURN_VALUE;
		ret->return_value.value = bin_op_index;
		ret->return_value.io_state = io_state_index;
	}

	{
		Instr* region = instr_buffer_at(instr_buffer, region_index);
		region->region.last_instr = return_index;
	}

	// Compute live ranges
	X64Register reg_configurations[3][3] = {
		{ X64_REG_A, X64_REG_C, X64_REG_D }, // 0 - left_operand, 1 - right_operand, 2 - bin_op
		{ X64_REG_A, X64_REG_C, X64_REG_A },
		{ X64_REG_A, X64_REG_C, X64_REG_C },
	};

	InstrStorageLocation* instr_storage = arena_alloc_array_zeroed(context->arena,
			InstrStorageLocation,
			instr_buffer->count);

	for (size_t i = 0; i < instr_buffer->count; i += 1) {
		instr_storage[i].kind = INSTR_STORAGE_NONE;
		instr_storage[i].reg = 0;
	}

	StringStorage string_storage = {};
	string_storage.allocator = panic_allocator_new();

	for (size_t i = 0; i < array_size(reg_configurations); i += 1) {
		X64CodeGenerator gen = {};
		gen.flags = X64_SKIP_REG_ALLOC | X64_PRINT_SCHEDULED_IR;
		gen.instr_buffer = *instr_buffer;
		gen.allocator = context->arena;
		gen.temp_allocator = context->temp_arena;
		gen.string_consts = str_storage_to_array(&string_storage);
		gen.instr_storage = instr_storage;

		AbiParam ret = { .kind = ABI_PARAM_NORMAL };
		gen.function_signature = (AbiSignature) { .call_conv = CALL_CONV_CDECL, .returns = &ret };

		{
			instr_storage[left_operand_index.value].kind = INSTR_STORAGE_REG;
			instr_storage[left_operand_index.value].reg = reg_configurations[i][0];

			instr_storage[right_operand_index.value].kind = INSTR_STORAGE_REG;
			instr_storage[right_operand_index.value].reg = reg_configurations[i][1];

			instr_storage[bin_op_index.value].kind = INSTR_STORAGE_REG;
			instr_storage[bin_op_index.value].reg = reg_configurations[i][2];
		}

		LoweredFunction lowered_function = x64_generate_code(&gen, region_index);
		MachineCodeBuffer machine_code = _lowered_code_to_machine_code(&lowered_function);
		
		typedef uint64_t(*Function)();

		Function function = (Function)machine_code.code;
		uint64_t result = function();

		assert(result == 96);

		free_executable(machine_code.code, machine_code.size_in_bytes);
	}

	instr_buffer_release(instr_buffer);
}

void test_imul_8_instr_code_gen_for_different_reg_configurations(TestContext* context) {
	Arena* instr_allocator = context->arena;

	InstrBuffer buffer = {};
	InstrBuffer* instr_buffer = &buffer;
	instr_buffer_init(instr_buffer, instr_allocator);

	// Setup out test program, which just computes 10 - 5 and returns the result.
	InstrIndex left_operand_index = instr_buffer_append(instr_buffer, instr_allocator);
	InstrIndex right_operand_index = instr_buffer_append(instr_buffer, instr_allocator);
	InstrIndex bin_op_index = instr_buffer_append(instr_buffer, instr_allocator);
	InstrIndex cast_index = instr_buffer_append(instr_buffer, instr_allocator);
	InstrIndex io_state_index = instr_buffer_append(instr_buffer, instr_allocator);
	InstrIndex return_index = instr_buffer_append(instr_buffer, instr_allocator);
	InstrIndex region_index = instr_new_region(instr_buffer, instr_allocator);

	Instr* left_operand = instr_buffer_at(instr_buffer, left_operand_index);
	Instr* right_operand = instr_buffer_at(instr_buffer, right_operand_index);
	Instr* bin_op = instr_buffer_at(instr_buffer, bin_op_index);

	{
		bin_op->bin_op.kind = INSTR_BIN_IMUL;
		bin_op->bin_op.left = left_operand_index;
		bin_op->bin_op.right = right_operand_index;
	}

	{
		Instr* cast = instr_buffer_at(instr_buffer, cast_index);
		cast->kind = INSTR_CAST_TO_64;
		cast->cast.value = bin_op_index;
	}

	{
		Instr* io_state = instr_buffer_at(instr_buffer, io_state_index);
		io_state->kind = INSTR_IO_STATE;
		io_state->io_state.producer = INVALID_INSTR_INDEX;
	}

	{
		Instr* ret = instr_buffer_at(instr_buffer, return_index);
		ret->kind = INSTR_RETURN_VALUE;
		ret->return_value.value = cast_index;
		ret->return_value.io_state = io_state_index;
	}

	{
		Instr* region = instr_buffer_at(instr_buffer, region_index);
		region->region.last_instr = return_index;
	}

	// Compute live ranges
	X64Register registers[] = { X64_REG_A, X64_REG_D, X64_REG_C };
	X64Register reg_configurations[3 * 3 * 3][4];

	// Generate all the permutations for the first 3 register locations.
	// The 4 one is always `X64_REG_B`
	uint32_t indices[3] = { 0, 0, 0 };
	for (size_t i = 0; i < 3 * 3 * 3; i += 1) {
		reg_configurations[i][0] = registers[indices[0]];
		reg_configurations[i][1] = registers[indices[1]];
		reg_configurations[i][2] = registers[indices[2]];
		reg_configurations[i][3] = X64_REG_B;

		uint32_t carry = 1;
		carry = (indices[2] += carry) / 3;
		carry = (indices[1] += carry) / 3;
		carry = (indices[0] += carry) / 3;

		indices[0] %= 3;
		indices[1] %= 3;
		indices[2] %= 3;
	}

	InstrStorageLocation* instr_storage = arena_alloc_array_zeroed(context->arena,
			InstrStorageLocation,
			instr_buffer->count);

	for (size_t i = 0; i < instr_buffer->count; i += 1) {
		instr_storage[i].kind = INSTR_STORAGE_NONE;
		instr_storage[i].reg = 0;
	}

	StringStorage string_storage = {};
	string_storage.allocator = panic_allocator_new();

	for (size_t bit_count_index = 0; bit_count_index < 4; bit_count_index += 1) {
		printf("Bit Count: %zu\n", (size_t)(1 << (bit_count_index + 3)));

		left_operand->kind = INSTR_CONST_8 + bit_count_index;
		right_operand->kind = INSTR_CONST_8 + bit_count_index;
		bin_op->kind = INSTR_BIN_OP_8 + bit_count_index;

		switch (bit_count_index) {
		case 0:
			left_operand->const_8.u = 16;
			right_operand->const_8.u = 9;
			break;
		case 1:
			left_operand->const_16.u = 16;
			right_operand->const_16.u = 9;
			break;
		case 2:
			left_operand->const_64.u = 16;
			right_operand->const_64.u = 9;
			break;
		case 3:
			left_operand->const_64.u = 16;
			right_operand->const_64.u = 9;
			break;
		}

		for (size_t i = 0; i < array_size(reg_configurations); i += 1) {
			if (reg_configurations[i][0] == reg_configurations[i][1]) {
				continue;
			}

			X64CodeGenerator gen = {};
			gen.flags = X64_SKIP_REG_ALLOC | X64_PRINT_SCHEDULED_IR;
			gen.instr_buffer = *instr_buffer;
			gen.allocator = context->arena;
			gen.temp_allocator = context->temp_arena;
			gen.string_consts = str_storage_to_array(&string_storage);
			gen.instr_storage = instr_storage;

			AbiParam ret = { .kind = ABI_PARAM_NORMAL };
			gen.function_signature = (AbiSignature) {
				.call_conv = CALL_CONV_CDECL,
				.returns = &ret
			};

			{
				instr_storage[left_operand_index.value].kind = INSTR_STORAGE_REG;
				instr_storage[left_operand_index.value].reg = reg_configurations[i][0];

				instr_storage[right_operand_index.value].kind = INSTR_STORAGE_REG;
				instr_storage[right_operand_index.value].reg = reg_configurations[i][1];

				instr_storage[bin_op_index.value].kind = INSTR_STORAGE_REG;
				instr_storage[bin_op_index.value].reg = reg_configurations[i][2];

				instr_storage[cast_index.value].kind = INSTR_STORAGE_REG;
				instr_storage[cast_index.value].reg = reg_configurations[i][3];
			}

			LoweredFunction lowered_function = x64_generate_code(&gen, region_index);
			MachineCodeBuffer machine_code = _lowered_code_to_machine_code(&lowered_function);
			
			typedef uint64_t(*Function)();

			Function function = (Function)machine_code.code;
			uint64_t result = function();

			assert(result == 144);

			free_executable(machine_code.code, machine_code.size_in_bytes);
		}
	}

	instr_buffer_release(instr_buffer);
}

void test_div_instr_code_gen_for_different_reg_configurations(TestContext* context) {
	Arena* instr_allocator = context->arena;

	InstrBuffer buffer = {};
	InstrBuffer* instr_buffer = &buffer;
	instr_buffer_init(instr_buffer, instr_allocator);

	InstrIndex left_operand_index = instr_buffer_append(instr_buffer, instr_allocator);
	InstrIndex right_operand_index = instr_buffer_append(instr_buffer, instr_allocator);
	InstrIndex bin_op_index = instr_buffer_append(instr_buffer, instr_allocator);
	InstrIndex cast_index = instr_buffer_append(instr_buffer, instr_allocator);
	InstrIndex io_state_index = instr_buffer_append(instr_buffer, instr_allocator);
	InstrIndex return_index = instr_buffer_append(instr_buffer, instr_allocator);
	InstrIndex region_index = instr_new_region(instr_buffer, instr_allocator);

	Instr* left_operand = instr_buffer_at(instr_buffer, left_operand_index);
	Instr* right_operand = instr_buffer_at(instr_buffer, right_operand_index);
	Instr* bin_op = instr_buffer_at(instr_buffer, bin_op_index);
	bin_op->bin_op.kind = INSTR_BIN_IDIV;
	bin_op->bin_op.left = left_operand_index;
	bin_op->bin_op.right = right_operand_index;

	{
		Instr* cast = instr_buffer_at(instr_buffer, cast_index);
		cast->kind = INSTR_CAST_TO_64;
		cast->cast.value = bin_op_index;
	}

	{
		Instr* io_state = instr_buffer_at(instr_buffer, io_state_index);
		io_state->kind = INSTR_IO_STATE;
		io_state->io_state.producer = INVALID_INSTR_INDEX;
	}

	{
		Instr* ret = instr_buffer_at(instr_buffer, return_index);
		ret->kind = INSTR_RETURN_VALUE;
		ret->return_value.value = cast_index;
		ret->return_value.io_state = io_state_index;
	}

	{
		Instr* region = instr_buffer_at(instr_buffer, region_index);
		region->region.last_instr = return_index;
	}

	X64Register registers[] = { X64_REG_A, X64_REG_D, X64_REG_C };
	X64Register reg_configurations[3 * 3 * 3][4];

	// Generate all the permutations for the first 3 register locations.
	// The 4 one is always `X64_REG_B`
	uint32_t indices[3] = { 0, 0, 0 };
	for (size_t i = 0; i < 3 * 3 * 3; i += 1) {
		reg_configurations[i][0] = registers[indices[0]];
		reg_configurations[i][1] = registers[indices[1]];
		reg_configurations[i][2] = registers[indices[2]];
		reg_configurations[i][3] = X64_REG_B;

		uint32_t carry = 1;
		carry = (indices[2] += carry) / 3;
		carry = (indices[1] += carry) / 3;
		carry = (indices[0] += carry) / 3;

		indices[0] %= 3;
		indices[1] %= 3;
		indices[2] %= 3;
	}

	InstrStorageLocation* instr_storage = arena_alloc_array_zeroed(context->arena,
			InstrStorageLocation,
			instr_buffer->count);

	for (size_t i = 0; i < instr_buffer->count; i += 1) {
		instr_storage[i].kind = INSTR_STORAGE_NONE;
		instr_storage[i].reg = 0;
	}

	for (size_t bit_count_index = 0; bit_count_index < 4; bit_count_index += 1) {
		printf("Bit Count: %zu\n", (size_t)(1 << (bit_count_index + 3)));

		left_operand->kind = INSTR_CONST_8 + bit_count_index;
		right_operand->kind = INSTR_CONST_8 + bit_count_index;
		bin_op->kind = INSTR_BIN_OP_8 + bit_count_index;

		switch (bit_count_index) {
		case 0:
			left_operand->const_8.u = 16;
			right_operand->const_8.u = 3;
			break;
		case 1:
			left_operand->const_16.u = 16;
			right_operand->const_16.u = 3;
			break;
		case 2:
			left_operand->const_64.u = 16;
			right_operand->const_64.u = 3;
			break;
		case 3:
			left_operand->const_64.u = 16;
			right_operand->const_64.u = 3;
			break;
		}
		
		for (size_t i = 0; i < array_size(reg_configurations); i += 1) {
			printf("Permutation: %zu\n", i);

			if (reg_configurations[i][0] == reg_configurations[i][1]) {
				continue;
			}

			X64CodeGenerator gen = {};
			gen.flags = X64_SKIP_REG_ALLOC;
			gen.instr_buffer = *instr_buffer;
			gen.allocator = context->arena;
			gen.temp_allocator = context->temp_arena;
			gen.string_consts = (StringArray) {};
			gen.instr_storage = instr_storage;

			AbiParam ret = { .kind = ABI_PARAM_NORMAL };
			gen.function_signature = (AbiSignature) {
				.call_conv = CALL_CONV_CDECL,
					.returns = &ret
			};

			{
				instr_storage[left_operand_index.value].kind = INSTR_STORAGE_REG;
				instr_storage[left_operand_index.value].reg = reg_configurations[i][0];

				instr_storage[right_operand_index.value].kind = INSTR_STORAGE_REG;
				instr_storage[right_operand_index.value].reg = reg_configurations[i][1];

				instr_storage[bin_op_index.value].kind = INSTR_STORAGE_REG;
				instr_storage[bin_op_index.value].reg = reg_configurations[i][2];

				instr_storage[cast_index.value].kind = INSTR_STORAGE_REG;
				instr_storage[cast_index.value].reg = reg_configurations[i][3];
			}

			LoweredFunction lowered_function = x64_generate_code(&gen, region_index);
			MachineCodeBuffer machine_code = _lowered_code_to_machine_code(&lowered_function);
			
			typedef uint64_t(*Function)();

			Function function = (Function)machine_code.code;
			uint64_t result = function();

			assert(result == 5);

			free_executable(machine_code.code, machine_code.size_in_bytes);
		}
	}

	instr_buffer_release(instr_buffer);
}

void test_mod_instr_code_gen_for_different_reg_configurations(TestContext* context) {
	Arena* instr_allocator = context->arena;

	InstrBuffer buffer = {};
	InstrBuffer* instr_buffer = &buffer;
	instr_buffer_init(instr_buffer, instr_allocator);

	InstrIndex left_operand_index = instr_buffer_append(instr_buffer, instr_allocator);
	InstrIndex right_operand_index = instr_buffer_append(instr_buffer, instr_allocator);
	InstrIndex bin_op_index = instr_buffer_append(instr_buffer, instr_allocator);
	InstrIndex cast_index = instr_buffer_append(instr_buffer, instr_allocator);
	InstrIndex io_state_index = instr_buffer_append(instr_buffer, instr_allocator);
	InstrIndex return_index = instr_buffer_append(instr_buffer, instr_allocator);
	InstrIndex region_index = instr_new_region(instr_buffer, instr_allocator);

	Instr* left_operand = instr_buffer_at(instr_buffer, left_operand_index);
	Instr* right_operand = instr_buffer_at(instr_buffer, right_operand_index);
	Instr* bin_op = instr_buffer_at(instr_buffer, bin_op_index);
	bin_op->bin_op.kind = INSTR_BIN_IMOD;
	bin_op->bin_op.left = left_operand_index;
	bin_op->bin_op.right = right_operand_index;

	{
		Instr* cast = instr_buffer_at(instr_buffer, cast_index);
		cast->kind = INSTR_CAST_TO_64;
		cast->cast.value = bin_op_index;
	}

	{
		Instr* io_state = instr_buffer_at(instr_buffer, io_state_index);
		io_state->kind = INSTR_IO_STATE;
		io_state->io_state.producer = INVALID_INSTR_INDEX;
	}

	{
		Instr* ret = instr_buffer_at(instr_buffer, return_index);
		ret->kind = INSTR_RETURN_VALUE;
		ret->return_value.value = cast_index;
		ret->return_value.io_state = io_state_index;
	}

	{
		Instr* region = instr_buffer_at(instr_buffer, region_index);
		region->region.last_instr = return_index;
	}

	// Compute live ranges
	X64Register registers[] = { X64_REG_A, X64_REG_D, X64_REG_C };
	X64Register reg_configurations[3 * 3 * 3][4];

	// Generate all the permutations for the first 3 register locations.
	// The 4 one is always `X64_REG_B`
	uint32_t indices[3] = { 0, 0, 0 };
	for (size_t i = 0; i < 3 * 3 * 3; i += 1) {
		reg_configurations[i][0] = registers[indices[0]];
		reg_configurations[i][1] = registers[indices[1]];
		reg_configurations[i][2] = registers[indices[2]];
		reg_configurations[i][3] = X64_REG_B;

		uint32_t carry = 1;
		carry = (indices[2] += carry) / 3;
		carry = (indices[1] += carry) / 3;
		carry = (indices[0] += carry) / 3;

		indices[0] %= 3;
		indices[1] %= 3;
		indices[2] %= 3;
	}

	InstrStorageLocation* instr_storage = arena_alloc_array_zeroed(context->arena,
			InstrStorageLocation,
			instr_buffer->count);

	for (size_t i = 0; i < instr_buffer->count; i += 1) {
		instr_storage[i].kind = INSTR_STORAGE_NONE;
		instr_storage[i].reg = 0;
	}

	for (size_t bit_count_index = 0; bit_count_index < 4; bit_count_index += 1) {
		printf("Bit Count: %zu\n", (size_t)(1 << (bit_count_index + 3)));

		left_operand->kind = INSTR_CONST_8 + bit_count_index;
		right_operand->kind = INSTR_CONST_8 + bit_count_index;
		bin_op->kind = INSTR_BIN_OP_8 + bit_count_index;

		switch (bit_count_index) {
		case 0:
			left_operand->const_8.u = 16;
			right_operand->const_8.u = 3;
			break;
		case 1:
			left_operand->const_16.u = 16;
			right_operand->const_16.u = 3;
			break;
		case 2:
			left_operand->const_64.u = 16;
			right_operand->const_64.u = 3;
			break;
		case 3:
			left_operand->const_64.u = 16;
			right_operand->const_64.u = 3;
			break;
		}
		
		for (size_t i = 0; i < array_size(reg_configurations); i += 1) {
			printf("Permutation: %zu\n", i);

			if (reg_configurations[i][0] == reg_configurations[i][1]) {
				continue;
			}

			X64CodeGenerator gen = {};
			gen.flags = X64_SKIP_REG_ALLOC;
			gen.instr_buffer = *instr_buffer;
			gen.allocator = context->arena;
			gen.temp_allocator = context->temp_arena;
			gen.string_consts = (StringArray) {};
			gen.instr_storage = instr_storage;

			AbiParam ret = { .kind = ABI_PARAM_NORMAL };
			gen.function_signature = (AbiSignature) {
				.call_conv = CALL_CONV_CDECL,
					.returns = &ret
			};

			{
				instr_storage[left_operand_index.value].kind = INSTR_STORAGE_REG;
				instr_storage[left_operand_index.value].reg = reg_configurations[i][0];

				instr_storage[right_operand_index.value].kind = INSTR_STORAGE_REG;
				instr_storage[right_operand_index.value].reg = reg_configurations[i][1];

				instr_storage[bin_op_index.value].kind = INSTR_STORAGE_REG;
				instr_storage[bin_op_index.value].reg = reg_configurations[i][2];

				instr_storage[cast_index.value].kind = INSTR_STORAGE_REG;
				instr_storage[cast_index.value].reg = reg_configurations[i][3];
			}

			LoweredFunction lowered_function = x64_generate_code(&gen, region_index);
			MachineCodeBuffer machine_code = _lowered_code_to_machine_code(&lowered_function);
			
			typedef uint64_t(*Function)();

			Function function = (Function)machine_code.code;
			uint64_t result = function();

			assert(result == 1);

			free_executable(machine_code.code, machine_code.size_in_bytes);
		}
	}

	instr_buffer_release(instr_buffer);
}

// From x64 backend
void _emit_bitwise_shift(CodeBuffer* buffer,
		MnemonicKind mnemonic,
		X64Register value_reg,
		X64Register count_reg,
		X64Register dst_reg,
		uint8_t bit_count,
		uint16_t allowed_temp_registers,
		Arena* allocator,
		Arena* temp_allocator);

static X64Register CDECL_CALLEE_SAVED[] = {
	X64_REG_B,
	X64_REG_BP,
	X64_REG_DI,
	X64_REG_SI,
	X64_REG_SP,
	X64_REG_12,
	X64_REG_13,
	X64_REG_14,
	X64_REG_15,
};

static const char* X64_REG_BASE_NAMES[] = {
	"RAX",
	"RCX",
	"RDX",
	"RBX",
	"RSP",
	"RBP",
	"RSI",
	"RDI",
	"R8",
	"R9",
	"R10",
	"R11",
	"R12",
	"R13",
	"R14",
	"R15",
};

// Tests that the generated bitwise shift instruction, during execution doesn't override the
// registers used by other instructions.
//
// This test go through all the potentially problematic register configurations of inputs and
// outputs, + all the bit count variants of the instruction.
void test_bitwise_shift_no_context_polution(TestContext* context) {
	// Compute live ranges
	X64Register registers[] = { X64_REG_A, X64_REG_D, X64_REG_C, X64_REG_12 };
	X64Register reg_configurations[4 * 4 * 4][4];

	// Generate all the permutations for the first 3 register locations.
	// The 4 one is always `X64_REG_B`
	uint32_t indices[3] = { 0, 0, 0 };
	for (size_t i = 0; i < 4 * 4 * 4; i += 1) {
		reg_configurations[i][0] = registers[indices[0]];
		reg_configurations[i][1] = registers[indices[1]];
		reg_configurations[i][2] = registers[indices[2]];
		reg_configurations[i][3] = X64_REG_B;

		uint32_t carry = 1;
		carry = (indices[2] += carry) / 4;
		carry = (indices[1] += carry) / 4;
		carry = (indices[0] += carry) / 4;

		indices[0] %= 4;
		indices[1] %= 4;
		indices[2] %= 4;
	}

	uint16_t disallowed_regs = (1 << X64_REG_SP) | (1 << X64_REG_BP) | (1 << X64_REG_SI) | (1 << X64_REG_DI);
	uint16_t temp_registers  = (1 << X64_REG_15);

	for (size_t bit_count_index = 0; bit_count_index < 4; bit_count_index += 1) {
		for (size_t i = 0; i < array_size(reg_configurations); i += 1) {
			X64Register value_reg = reg_configurations[i][0];
			X64Register count_reg = reg_configurations[i][1];
			X64Register dst_reg   = reg_configurations[i][2];

			if (value_reg == count_reg) {
				continue;
			}

			uint8_t bit_count = 1 << (bit_count_index + 3);
			uint8_t reg_size  = 1 << bit_count_index;

			printf("permutation: %zu value: %s count: %s dst: %s bit_count: %u\n",
					i,
					X64_REG_BASE_NAMES[value_reg],
					X64_REG_BASE_NAMES[count_reg],
					X64_REG_BASE_NAMES[dst_reg],
					(uint32_t)bit_count);
			fflush(stdout);

			CodeBuffer buffer;
			code_buffer_init(&buffer, context->temp_arena);

			for (size_t i = 0; i < array_size(CDECL_CALLEE_SAVED); i += 1) {
				encode_1(&buffer, MNEMONIC_PUSH, operand_reg(CDECL_CALLEE_SAVED[i], 64));
			}

			for (X64Register r = 0; r < X64_REG_COUNT; r += 1) {
				if (r == X64_REG_SP || r == X64_REG_BP || r == X64_REG_SI || r == X64_REG_DI) {
					continue;
				}

				if (has_flag(temp_registers, 1 << r)) {
					continue;
				}

				if (r == value_reg) {
					encode_2(&buffer,
							MNEMONIC_MOV,
							operand_reg(r, bit_count),
							operand_imm(0xa, bit_count));
				} else if (r == count_reg) {
					encode_2(&buffer,
							MNEMONIC_MOV,
							operand_reg(r, bit_count),
							operand_imm(0x2, bit_count));
				} else if (r == dst_reg) {
					continue;
				} else {
					encode_2(&buffer,
							MNEMONIC_MOV,
							operand_reg(r, bit_count),
							operand_imm(0xdeadbeefdeadbeef, bit_count));
				}
			}

			_emit_bitwise_shift(&buffer,
					MNEMONIC_SHL,
					value_reg,
					count_reg,
					dst_reg,
					bit_count,
					temp_registers,
					// ~(disallowed_regs | (1 << value_reg) | (1 << count_reg) | (1 << dst_reg)),
					context->arena,
					context->temp_arena);

			uint32_t stack_usage = 0;
			for (X64Register r = 0; r < X64_REG_COUNT; r += 1) {
				if (r == X64_REG_SP || r == X64_REG_BP || r == X64_REG_SI || r == X64_REG_DI) {
					continue;
				}

				if (has_flag(temp_registers, 1 << r)) {
					continue;
				}

				encode_1(&buffer, MNEMONIC_PUSH, operand_reg(r, 64));
				stack_usage += 8;
			}

			uint32_t stack_offset = 0;
			for (X64Register r = 0; r < X64_REG_COUNT; r += 1) {
				if (r == X64_REG_SP || r == X64_REG_BP || r == X64_REG_SI || r == X64_REG_DI) {
					continue;
				}

				if (has_flag(temp_registers, 1 << r)) {
					continue;
				}

				uint64_t expected_imm = 0xdeadbeefdeadbeef;

				if (r == value_reg) {
					expected_imm = 0xa;
				} else if (r == count_reg) {
					expected_imm = 0x2;
				}

				if (r == dst_reg) {
					expected_imm = 0xa << 0x2;
				}

				encode_2(&buffer,
						MNEMONIC_MOV,
						operand_reg(X64_REG_A, bit_count),
						operand_imm(expected_imm, bit_count));

				encode_2(&buffer,
						MNEMONIC_MOV,
						operand_reg(X64_REG_C, bit_count),
						operand_stack_mem(stack_usage - stack_offset - 8, bit_count));

				encode_2(&buffer,
						MNEMONIC_CMP,
						operand_reg(X64_REG_C, bit_count),
						operand_reg(X64_REG_A, bit_count));

				encode_1(&buffer, MNEMONIC_JZ, operand_rel32(1));

				encode_n(&buffer, MNEMONIC_INT3, NULL, 0);

				stack_offset += 8;
			}

			encode_2(&buffer,
					MNEMONIC_ADD,
					operand_reg(X64_REG_SP, 64),
					operand_imm(stack_usage, 32));

			for (size_t i = array_size(CDECL_CALLEE_SAVED); i > 0; i -= 1) {
				encode_1(&buffer, MNEMONIC_POP, operand_reg(CDECL_CALLEE_SAVED[i - 1], 64));
			}

			encode_n(&buffer, MNEMONIC_RET, NULL, 0);

			typedef void(*Function)();

			void* code = allocate_executable(buffer.size);
			memcpy(code, buffer.buffer, buffer.size);
			Function function = (Function)code;

			function();

			free_executable(code, buffer.size);
		}
	}
}

static uint64_t _internal_store(uint64_t* out) {
	*out = 10;
	return 0;
}

static void _resolve_memory_operation_symbols(SymbolMap* map, void* data) {
	symbol_map_insert_dynamically_linked_impl(map, STR_LIT("store"), _internal_store);
}

void test_memory_operations_are_synchronized_with_calls(TestContext* context) {
	String source_code = STR_LIT(
			"typedef unsigned long long uint64_t;\n"
			"__declspec(dllimport) uint64_t store(uint64_t* out);\n"
			"uint64_t main(uint64_t* out) {\n"
			"    uint64_t value = *out;\n"
			"    store(out);\n"
			"    return value;\n"
			"}\n");
	MachineCodeBuffer machine_code = _compile_with_custom_symbols(context,
			source_code,
			_resolve_memory_operation_symbols,
			NULL);

	typedef uint64_t(*Function)(uint64_t*);

	Function function = (Function)machine_code.code;

	uint64_t input = 100;
	assert(function(&input) == 100);

	free_executable(machine_code.code, machine_code.size_in_bytes);
}

void test_ptr_store_instr(TestContext* context) {
	String source_code = STR_LIT(
			"typedef unsigned long long uint64_t;\n"
			"uint64_t main(uint64_t* out) {\n"
			"    *out = 100;\n"
			"    return 0;\n"
			"}\n");

	MachineCodeBuffer machine_code = _compile(context, source_code);

	typedef uint64_t(*Function)(uint64_t*);

	Function function = (Function)machine_code.code;

	uint64_t input = 40;
	function(&input);
	assert(input == 100);

	free_executable(machine_code.code, machine_code.size_in_bytes);
}

void test_ptr_store_synced_with_calls(TestContext* context) {
	String source_code = STR_LIT(
			"typedef unsigned long long uint64_t;\n"
			"__declspec(dllimport) uint64_t store(uint64_t* out);\n"
			"uint64_t main(uint64_t* out) {\n"
			"    store(out);\n"
			"    *out = 100;\n"
			"    return 0;\n"
			"}\n");

	MachineCodeBuffer machine_code = _compile_with_custom_symbols(context,
			source_code,
			_resolve_memory_operation_symbols,
			NULL);

	typedef uint64_t(*Function)(uint64_t*);

	Function function = (Function)machine_code.code;

	uint64_t input = 40;
	function(&input);
	assert(input == 100);

	free_executable(machine_code.code, machine_code.size_in_bytes);
}

void test_array_element_assignment(TestContext* context) {
	String source_code = STR_LIT(
			"typedef unsigned long long uint64_t;\n"
			"uint64_t main(uint64_t* out) {\n"
			"    out[0] = 100;\n"
			"    return 0;\n"
			"}\n");

	MachineCodeBuffer machine_code = _compile(context, source_code);

	typedef uint64_t(*Function)(uint64_t*);

	Function function = (Function)machine_code.code;

	uint64_t input = 0;
	function(&input);
	assert(input == 100);

	free_executable(machine_code.code, machine_code.size_in_bytes);
}

void test_encode_mov_indirect_addr(TestContext* context) {
	uint8_t expected[] = { 0x48, 0x8b, 0x02 };

	CodeBuffer buffer;
	code_buffer_init(&buffer, context->arena);

	encode_2(&buffer,
			MNEMONIC_MOV,
			operand_reg(X64_REG_A, 64),
			operand_mem(X64_REG_D, 64));

	assert(buffer.size == array_size(expected));
	assert_msg(memcmp(buffer.buffer, expected, buffer.size) == 0, "mov rax, [rdx]");
}

void test_encode_addressing_of_r12_r13_and_bp(TestContext* context) {
	{
		uint8_t expected[] = { 0x4D, 0x89, 0x65, 0x00 };

		CodeBuffer buffer;
		code_buffer_init(&buffer, context->arena);

		encode_2(&buffer,
				MNEMONIC_MOV,
				operand_mem(X64_REG_13, 64),
				operand_reg(X64_REG_12, 64));

		assert(buffer.size == array_size(expected));
		assert_msg(memcmp(buffer.buffer, expected, buffer.size) == 0, "mov [r13], r12");
	}

	{
		uint8_t expected[] = { 0x4D, 0x8B, 0x65, 0x00 };

		CodeBuffer buffer;
		code_buffer_init(&buffer, context->arena);

		encode_2(&buffer,
				MNEMONIC_MOV,
				operand_reg(X64_REG_12, 64),
				operand_mem(X64_REG_13, 64));

		assert(buffer.size == array_size(expected));
		assert_msg(memcmp(buffer.buffer, expected, buffer.size) == 0, "mov r12, [r13]");
	}

	{
		uint8_t expected[] = { 0x4C, 0x89, 0x65, 0x00 };

		CodeBuffer buffer;
		code_buffer_init(&buffer, context->arena);

		encode_2(&buffer,
				MNEMONIC_MOV,
				operand_mem(X64_REG_BP, 64),
				operand_reg(X64_REG_12, 64));

		assert(buffer.size == array_size(expected));
		assert_msg(memcmp(buffer.buffer, expected, buffer.size) == 0, "mov [rbp], r12");
	}

	{
		uint8_t expected[] = { 0x4C, 0x8B, 0x65, 0x00 };

		CodeBuffer buffer;
		code_buffer_init(&buffer, context->arena);

		encode_2(&buffer,
				MNEMONIC_MOV,
				operand_reg(X64_REG_12, 64),
				operand_mem(X64_REG_BP, 64));

		assert(buffer.size == array_size(expected));
		assert_msg(memcmp(buffer.buffer, expected, buffer.size) == 0, "mov r12, [rbp]");
	}

	{
		uint8_t expected[] = { 0x4D, 0x89, 0x0C, 0x24 };

		CodeBuffer buffer;
		code_buffer_init(&buffer, context->arena);

		encode_2(&buffer,
				MNEMONIC_MOV,
				operand_mem(X64_REG_12, 64),
				operand_reg(X64_REG_9, 64));

		assert(buffer.size == array_size(expected));
		assert_msg(memcmp(buffer.buffer, expected, buffer.size) == 0, "mov [r12], r9");
	}

	{
		uint8_t expected[] = { 0x4D, 0x8B, 0x0C, 0x24 };

		CodeBuffer buffer;
		code_buffer_init(&buffer, context->arena);

		encode_2(&buffer,
				MNEMONIC_MOV,
				operand_reg(X64_REG_9, 64),
				operand_mem(X64_REG_12, 64));

		assert(buffer.size == array_size(expected));
		assert_msg(memcmp(buffer.buffer, expected, buffer.size) == 0, "mov r9, [r12]");
	}
}

void test_encode_mov_const_32_to_extended_register(TestContext* context) {
	uint8_t expected[] = { 0x41, 0xb8, 0x6d, 0x0, 0x0, 0x0 };

	CodeBuffer buffer;
	code_buffer_init(&buffer, context->arena);

	encode_2(&buffer,
			MNEMONIC_MOV,
			operand_reg(X64_REG_8, 32),
			operand_imm(0x6d, 32));

	assert(buffer.size == array_size(expected));
	assert_msg(memcmp(buffer.buffer, expected, buffer.size) == 0, "mov r8d, 0x6d");
}

void test_encode_push_extended_register(TestContext* context) {
	// NOTE: REX.W seems to be ignored here, however the encoding algorithm prefers to set it, so
	//       test for that.
	//
	//       Without REX.W set the encoded bytes should be 0x41, 0x50
	uint8_t expected[] = { 0x49, 0x50 };

	CodeBuffer buffer;
	code_buffer_init(&buffer, context->arena);

	encode_1(&buffer,
			MNEMONIC_PUSH,
			operand_reg(X64_REG_8, 64));

	assert(buffer.size == array_size(expected));
	assert_msg(memcmp(buffer.buffer, expected, buffer.size) == 0, "push r8");
}

void test_encode_pop_extended_register(TestContext* context) {
	// NOTE: REX.W seems to be ignored here, however the encoding algorithm prefers to set it, so
	//       test for that.
	//
	//       Without REX.W set the encoded bytes should be 0x41, 0x58
	uint8_t expected[] = { 0x49, 0x58 };

	CodeBuffer buffer;
	code_buffer_init(&buffer, context->arena);

	encode_1(&buffer,
			MNEMONIC_POP,
			operand_reg(X64_REG_8, 64));

	assert(buffer.size == array_size(expected));
	assert_msg(memcmp(buffer.buffer, expected, buffer.size) == 0, "pop r8");
}

void test_encode_movzx_16_to_32_bits(TestContext* context) {
	CodeBuffer buffer;
	code_buffer_init(&buffer, context->arena);

	// Here `rax` and `rcx` are caller saved.
	// So we don't need to do any saving/restoring.
	encode_2(&buffer, MNEMONIC_MOV, operand_reg(X64_REG_A, 64), operand_imm(UINT64_MAX, 64));
	encode_2(&buffer, MNEMONIC_MOV, operand_reg(X64_REG_C, 16), operand_imm(0xa, 16));
	encode_2(&buffer, MNEMONIC_MOVZX, operand_reg(X64_REG_A, 32), operand_reg(X64_REG_C, 16));
	encode_n(&buffer, MNEMONIC_RET, NULL, 0);

	void* code = allocate_executable(buffer.size);
	memcpy(code, buffer.buffer, buffer.size);

	typedef uint64_t(*Function)();

	Function function = (Function)code;
	uint64_t result = function();

	free_executable(code, buffer.size);

	assert((result & 0xffffffff) == 0xa);
}

void test_encode_movsx_16_to_32_bits(TestContext* context) {
	CodeBuffer buffer;
	code_buffer_init(&buffer, context->arena);

	// Here `rax` and `rcx` are caller saved.
	// So we don't need to do any saving/restoring.
	encode_2(&buffer, MNEMONIC_MOV, operand_reg(X64_REG_A, 64), operand_imm(0, 64));
	encode_2(&buffer, MNEMONIC_MOV, operand_reg(X64_REG_C, 16), operand_imm(0xfffa, 16));
	encode_2(&buffer, MNEMONIC_MOVSX, operand_reg(X64_REG_A, 32), operand_reg(X64_REG_C, 16));
	encode_n(&buffer, MNEMONIC_RET, NULL, 0);

	void* code = allocate_executable(buffer.size);
	memcpy(code, buffer.buffer, buffer.size);

	typedef uint64_t(*Function)();

	Function function = (Function)code;
	uint64_t result = function();

	free_executable(code, buffer.size);

	assert((result & 0xffffffff) == 0xfffffffa);
}


void test_parallel_moves_produces_no_moves_if_input_locs_equal_expected_locs(TestContext* context) {
	X64Register expected_locs[] = { X64_REG_A, X64_REG_8, X64_REG_C, X64_REG_D };
	InstrStorageLocation input_locs[array_size(expected_locs)];

	for (size_t i = 0; i < array_size(expected_locs); i += 1) {
		input_locs[i].kind = INSTR_STORAGE_REG;
		input_locs[i].reg = expected_locs[i];
	}

	RegisterMoveArray moves = _parallel_move_values(input_locs,
			expected_locs,
			array_size(expected_locs),
			0,
			context->arena,
			context->temp_arena);

	assert(moves.count == 0);
}

static void _validate_parallel_moves(InstrStorageLocation* input_locs,
		X64Register* expected_locs,
		size_t loc_count,
		RegisterMoveArray moves,
		Arena* temp_allocator) {
	X64Register state[X64_REG_COUNT];
	memset(state, 0xff, sizeof(state));

	// Initial state
	for (size_t i = 0; i < loc_count; i += 1) {
		assert(input_locs[i].kind == INSTR_STORAGE_REG);
		state[input_locs[i].reg] = (uint16_t)input_locs[i].reg;
	}

	// Simulate the moves
	for (size_t i = 0; i < moves.count; i += 1) {
		RegisterMove move = moves.moves[i];
		
		state[move.dst] = state[move.src];
	}

	// Assert that all the inputs are in the corresponding expected location
	bool result = true;
	for (size_t i = 0; i < loc_count; i += 1) {
		if (state[expected_locs[i]] != (uint16_t)input_locs[i].reg) {
			printf("Expected input '%u' to be at location '%u', but found '%u'\n",
					(uint32_t)input_locs[i].reg,
					(uint32_t)expected_locs[i],
					(uint32_t)state[expected_locs[i]]);
			result = false;
		}
	}

	assert(result);
}

void test_parallel_moves_is_correct_for_input_in_shifted_locations(TestContext* context) {
	X64Register expected_locs[X64_REG_COUNT];
	InstrStorageLocation input_locs[array_size(expected_locs)];

	uint16_t shift = (uint16_t)rand();

	for (size_t i = 0; i < array_size(expected_locs); i += 1) {
		expected_locs[i] = (i + shift) % X64_REG_COUNT;

		input_locs[i].kind = INSTR_STORAGE_REG;
		input_locs[i].reg = expected_locs[i];
	}

	RegisterMoveArray moves = _parallel_move_values(input_locs,
			expected_locs,
			array_size(expected_locs),
			0,
			context->arena,
			context->temp_arena);

	_validate_parallel_moves(input_locs,
			expected_locs,
			array_size(expected_locs),
			moves,
			context->temp_arena);
}

void test_parallel_moves_cycle(TestContext* context) {
	X64Register expected_locs[] = { X64_REG_A, X64_REG_8 };
	InstrStorageLocation input_locs[array_size(expected_locs)];

	input_locs[0].kind = INSTR_STORAGE_REG;
	input_locs[0].reg = expected_locs[1];

	input_locs[1].kind = INSTR_STORAGE_REG;
	input_locs[1].reg = expected_locs[0];

	RegisterMoveArray moves = _parallel_move_values(input_locs,
			expected_locs,
			array_size(expected_locs),
			(1 << X64_REG_C),
			context->arena,
			context->temp_arena);

	assert(moves.count == 3);

	_validate_parallel_moves(input_locs,
			expected_locs,
			array_size(expected_locs),
			moves,
			context->temp_arena);
}

void test_parallel_moves_multiple_cycles(TestContext* context) {
	X64Register expected_locs[] = { X64_REG_A, X64_REG_8, X64_REG_C, X64_REG_D };
	InstrStorageLocation input_locs[array_size(expected_locs)];

	input_locs[0].kind = INSTR_STORAGE_REG;
	input_locs[0].reg = expected_locs[1];

	input_locs[1].kind = INSTR_STORAGE_REG;
	input_locs[1].reg = expected_locs[0];

	input_locs[2].kind = INSTR_STORAGE_REG;
	input_locs[2].reg = expected_locs[3];

	input_locs[3].kind = INSTR_STORAGE_REG;
	input_locs[3].reg = expected_locs[2];

	RegisterMoveArray moves = _parallel_move_values(input_locs,
			expected_locs,
			array_size(expected_locs),
			(1 << X64_REG_13),
			context->arena,
			context->temp_arena);

	assert(moves.count == 6);

	_validate_parallel_moves(input_locs,
			expected_locs,
			array_size(expected_locs),
			moves,
			context->temp_arena);
}

void test_parallel_move_same_value_into_multiple_locations(TestContext* context) {
	InstrStorageLocation input_locs[] = {
		(InstrStorageLocation) {
			.kind = INSTR_STORAGE_REG,
			.reg = X64_REG_C,
		},
		(InstrStorageLocation) {
			.kind = INSTR_STORAGE_REG,
			.reg = X64_REG_C,
		},
	};

	X64Register expected_locs[] = { X64_REG_A, X64_REG_B };

	RegisterMoveArray moves = _parallel_move_values(input_locs,
			expected_locs,
			array_size(expected_locs),
			0,
			context->arena,
			context->temp_arena);

	assert(moves.count == 2);

	_validate_parallel_moves(input_locs,
			expected_locs,
			array_size(expected_locs),
			moves,
			context->temp_arena);
}

void test_x64_compute_frame_layout_4_normal_args_no_return(TestContext* context) {
	AbiParam params[] = { 
		(AbiParam) { .kind = ABI_PARAM_NORMAL },
		(AbiParam) { .kind = ABI_PARAM_NORMAL },
		(AbiParam) { .kind = ABI_PARAM_NORMAL },
		(AbiParam) { .kind = ABI_PARAM_NORMAL },
	};

	AbiSignature signature = {
		.call_conv = CALL_CONV_CDECL,
		.param_count = array_size(params),
		.params = params,
		.returns = NULL
	};

	CallFrameLayout layout = compute_call_frame_layout(&signature, context->arena);
	assert(layout.location_count == 4);

	assert(layout.locations[0].kind == INSTR_STORAGE_REG);
	assert(layout.locations[0].reg  == X64_REG_C);

	assert(layout.locations[1].kind == INSTR_STORAGE_REG);
	assert(layout.locations[1].reg  == X64_REG_D);

	assert(layout.locations[2].kind == INSTR_STORAGE_REG);
	assert(layout.locations[2].reg  == X64_REG_8);

	assert(layout.locations[3].kind == INSTR_STORAGE_REG);
	assert(layout.locations[3].reg  == X64_REG_9);
}

void test_x64_compute_frame_layout_4_normal_args_return_normal(TestContext* context) {
	AbiParam params[] = { 
		(AbiParam) { .kind = ABI_PARAM_NORMAL },
		(AbiParam) { .kind = ABI_PARAM_NORMAL },
		(AbiParam) { .kind = ABI_PARAM_NORMAL },
		(AbiParam) { .kind = ABI_PARAM_NORMAL },
	};

	AbiParam returns = { .kind = ABI_PARAM_NORMAL };

	AbiSignature signature = {
		.call_conv = CALL_CONV_CDECL,
		.param_count = array_size(params),
		.params = params,
		.returns = &returns, 
	};

	CallFrameLayout layout = compute_call_frame_layout(&signature, context->arena);
	assert(layout.location_count == 4);

	assert(layout.locations[0].kind == INSTR_STORAGE_REG);
	assert(layout.locations[0].reg  == X64_REG_C);

	assert(layout.locations[1].kind == INSTR_STORAGE_REG);
	assert(layout.locations[1].reg  == X64_REG_D);

	assert(layout.locations[2].kind == INSTR_STORAGE_REG);
	assert(layout.locations[2].reg  == X64_REG_8);

	assert(layout.locations[3].kind == INSTR_STORAGE_REG);
	assert(layout.locations[3].reg  == X64_REG_9);
}

void test_x64_compute_frame_layout_4_normal_args_return_small_struct(TestContext* context) {
	AbiParam params[] = { 
		(AbiParam) { .kind = ABI_PARAM_NORMAL },
		(AbiParam) { .kind = ABI_PARAM_NORMAL },
		(AbiParam) { .kind = ABI_PARAM_NORMAL },
	};

	AbiParam returns = { .kind = ABI_PARAM_STRUCT, .struct_size = 8 };

	AbiSignature signature = {
		.call_conv = CALL_CONV_CDECL,
		.param_count = array_size(params),
		.params = params,
		.returns = &returns, 
	};

	CallFrameLayout layout = compute_call_frame_layout(&signature, context->arena);
	assert(layout.location_count == 3);

	assert(layout.locations[0].kind == INSTR_STORAGE_REG);
	assert(layout.locations[0].reg  == X64_REG_C);

	assert(layout.locations[1].kind == INSTR_STORAGE_REG);
	assert(layout.locations[1].reg  == X64_REG_D);

	assert(layout.locations[2].kind == INSTR_STORAGE_REG);
	assert(layout.locations[2].reg  == X64_REG_8);
}

void test_x64_compute_frame_layout_4_normal_args_return_large_struct(TestContext* context) {
	AbiParam params[] = { 
		(AbiParam) { .kind = ABI_PARAM_RETURN_LOCATION },
		(AbiParam) { .kind = ABI_PARAM_NORMAL },
		(AbiParam) { .kind = ABI_PARAM_NORMAL },
		(AbiParam) { .kind = ABI_PARAM_NORMAL },
	};

	AbiParam returns = { .kind = ABI_PARAM_STRUCT, .struct_size = 32 };

	AbiSignature signature = {
		.call_conv = CALL_CONV_CDECL,
		.param_count = array_size(params),
		.params = params,
		.returns = &returns, 
	};

	CallFrameLayout layout = compute_call_frame_layout(&signature, context->arena);
	assert(layout.location_count == 4);

	assert(layout.locations[0].kind == INSTR_STORAGE_REG);
	assert(layout.locations[0].reg  == X64_REG_C);

	assert(layout.locations[1].kind == INSTR_STORAGE_REG);
	assert(layout.locations[1].reg  == X64_REG_D);

	assert(layout.locations[2].kind == INSTR_STORAGE_REG);
	assert(layout.locations[2].reg  == X64_REG_8);

	assert(layout.locations[3].kind == INSTR_STORAGE_REG);
	assert(layout.locations[3].reg  == X64_REG_9);
}

void test_x64_compute_frame_layout_2_normal_2_struct_args_no_return(TestContext* context) {
	AbiParam params[] = { 
		(AbiParam) { .kind = ABI_PARAM_NORMAL },
		(AbiParam) { .kind = ABI_PARAM_NORMAL },
		(AbiParam) { .kind = ABI_PARAM_STRUCT, .struct_size = 96 },
		(AbiParam) { .kind = ABI_PARAM_STRUCT, .struct_size = 32 },
	};

	AbiSignature signature = {
		.call_conv = CALL_CONV_CDECL,
		.param_count = array_size(params),
		.params = params,
		.returns = NULL
	};

	CallFrameLayout layout = compute_call_frame_layout(&signature, context->arena);
	assert(layout.location_count == 4);

	assert(layout.locations[0].kind == INSTR_STORAGE_REG);
	assert(layout.locations[0].reg  == X64_REG_C);

	assert(layout.locations[1].kind == INSTR_STORAGE_REG);
	assert(layout.locations[1].reg  == X64_REG_D);

	assert(layout.locations[2].kind == INSTR_STORAGE_REG);
	assert(layout.locations[2].reg  == X64_REG_8);

	assert(layout.locations[3].kind == INSTR_STORAGE_REG);
	assert(layout.locations[3].reg  == X64_REG_9);
}

typedef struct {
	size_t a;
	int b;
	int c;
} StructArgument;

typedef struct {
	int a;
} SmallStructArgument;

static void _verify_struct_argument(StructArgument s) {
	assert(s.a == 0xdeadbeefdeadbeef);
	assert(s.b == 0xffaaccdd);
	assert(s.c == 0xddeeffaa);
}

static void _verify_small_struct_argument(SmallStructArgument s) {
	assert(s.a == 0xbeefbeef);
}

static StructArgument _return_struct() {
	return (StructArgument) { 0xbeefbeefaaddeecc, 0xaaeeffee, 0xaa0044dd };
}

static SmallStructArgument _return_small_struct() {
	return (SmallStructArgument) { 0xaaddeecc };
}

static void _verify_struct_argument_passed_through_stack(int a,
		int b,
		int c,
		int d,
		int e,
		int f,
		int g,
		int h,
		StructArgument s) {
	printf("%d %d %d %d %d %d %d %d\n", a, b, c, d, e, f, g, h);
	assert(a == 0);
	assert(b == 1);
	assert(c == 2);
	assert(d == 3);
	assert(e == 4);
	assert(f == 5);
	assert(g == 6);
	assert(h == 7);

	assert(s.a == 8);
	assert(s.b == 9);
	assert(s.c == 10);
}

static void _verify_small_struct_argument_passed_through_stack(int a,
		int b,
		int c,
		int d,
		int e,
		int f,
		int g,
		int h,
		SmallStructArgument s) {
	printf("%d %d %d %d %d %d %d %d\n", a, b, c, d, e, f, g, h);
	assert(a == 0);
	assert(b == 1);
	assert(c == 2);
	assert(d == 3);
	assert(e == 4);
	assert(f == 5);
	assert(g == 6);
	assert(h == 7);

	assert(s.a == 8);
}

static void _resolve_struct_argument_helpers(SymbolMap* map, void* data) {
	symbol_map_insert_dynamically_linked_impl(map,
			STR_LIT("_verify_struct_argument"),
			_verify_struct_argument);
	symbol_map_insert_dynamically_linked_impl(map,
			STR_LIT("_verify_small_struct_argument"),
			_verify_small_struct_argument);
	symbol_map_insert_dynamically_linked_impl(map,
			STR_LIT("_return_struct"),
			_return_struct);
	symbol_map_insert_dynamically_linked_impl(map,
			STR_LIT("_return_small_struct"),
			_return_small_struct);
	symbol_map_insert_dynamically_linked_impl(map,
			STR_LIT("_verify_small_struct_argument_passed_through_stack"),
			_verify_small_struct_argument_passed_through_stack);
	symbol_map_insert_dynamically_linked_impl(map,
			STR_LIT("_verify_struct_argument_passed_through_stack"),
			_verify_struct_argument_passed_through_stack);
}

void test_call_function_with_struct_argument(TestContext* context) {
	String source_code = STR_LIT(
			"typedef struct { size_t a; int b; int c; } StructArgument;\n"
			"typedef struct { int a; } SmallStructArgument;\n"
			"\n"
			"__declspec(dllimport) void _verify_struct_argument(StructArgument);\n"
			"__declspec(dllimport) void _verify_small_struct_argument(SmallStructArgument);\n"
			"void main() {\n"
			"    _verify_struct_argument((StructArgument) {\n"
			"        0xdeadbeefdeadbeef,"
			"        0xffaaccdd,"
			"        0xddeeffaa"
			"    });\n"
			"\n"
			"    _verify_small_struct_argument((SmallStructArgument) {\n"
			"        0xbeefbeef,"
			"    });\n"
			"}\n");

	MachineCodeBuffer machine_code = _compile_with_custom_symbols(context,
			source_code,
			_resolve_struct_argument_helpers,
			NULL);

	typedef void(*Function)();

	Function executable_function = (Function)machine_code.code;
	executable_function();
	free_executable(machine_code.code, machine_code.size_in_bytes);
}

void test_consume_struct_returned_from_call(TestContext* context) {
	String source_code = STR_LIT(
			"__declspec(dllimport) void assert(unsigned long long);\n"
			"\n"
			"typedef struct { size_t a; int b; int c; } Struct;\n"
			"typedef struct { int a; } SmallStruct;\n"
			"\n"
			"__declspec(dllimport) Struct _return_struct();\n"
			"__declspec(dllimport) SmallStruct _return_small_struct();\n"
			"\n"
			"void main() {\n"
			"    Struct s0 = _return_struct();\n"
			"    assert(s0.a == 0xbeefbeefaaddeecc);\n"
			"    assert(s0.b == 0xaaeeffee);\n"
			"    assert(s0.c == 0xaa0044dd);\n"
			"\n"
			"    SmallStruct s1 = _return_small_struct();\n"
			"    assert(s1.a == 0xaaddeecc);\n"
			"}\n");

	MachineCodeBuffer machine_code = _compile_with_custom_symbols(context,
			source_code,
			_resolve_struct_argument_helpers,
			NULL);

	typedef void(*Function)();

	Function executable_function = (Function)machine_code.code;
	executable_function();
	free_executable(machine_code.code, machine_code.size_in_bytes);
}

void test_call_function_with_struct_arg_passed_through_stack(TestContext* context) {
	String source_code = STR_LIT(
			"__declspec(dllimport) void assert(unsigned long long);\n"
			"\n"
			"typedef struct { size_t a; int b; int c; } Struct;\n"
			"typedef struct { int a; } SmallStruct;\n"
			"\n"
			"__declspec(dllimport) void _verify_small_struct_argument_passed_through_stack(int a,\n"
			"	int b,\n"
			"	int c,\n"
			"	int d,\n"
			"	int e,\n"
			"	int f,\n"
			"	int g,\n"
			"	int h,\n"
			"	SmallStruct s);\n"
			"\n"
			"__declspec(dllimport) void _verify_struct_argument_passed_through_stack(int a,\n"
			"	int b,\n"
			"	int c,\n"
			"	int d,\n"
			"	int e,\n"
			"	int f,\n"
			"	int g,\n"
			"	int h,\n"
			"	Struct s);\n"
			"\n"
			"__declspec(dllimport) Struct _return_struct();\n"
			"__declspec(dllimport) SmallStruct _return_small_struct();\n"
			"\n"
			"void main() {\n"
			"    _verify_small_struct_argument_passed_through_stack(0, 1, 2, 3, 4,\n"
			"		5, 6, 7, (SmallStruct) { 8 });\n"
			"    _verify_struct_argument_passed_through_stack(0, 1, 2, 3, 4,\n"
			"		5, 6, 7, (Struct) { 8, 9, 10 });\n"
			"}\n");

	MachineCodeBuffer machine_code = _compile_with_custom_symbols(context,
			source_code,
			_resolve_struct_argument_helpers,
			NULL);

	typedef void(*Function)();

	Function executable_function = (Function)machine_code.code;
	executable_function();
	free_executable(machine_code.code, machine_code.size_in_bytes);
}
