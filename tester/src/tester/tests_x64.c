#include "tests_x64.h"

#include "compiler/compiler.h"
#include "parser/preprocessor.h"
#include "parser/parser.h"
#include "code_gen/backends/x64.h"

#define DEFAULT_SOURCE_FILE_PATH "test.c"

typedef uint64_t(*ExecutableFunction)();
typedef void(*ResolverFunction)(FunctionRefTable* table, void* data);

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

	Parser parser = {};
	parser_init(&parser, &ast_arena, context->arena, &ident_storage, &preprocessor, &diagnostics);

	preprocessor_release(&preprocessor);

	AST parsed_ast = {};
	parser_parse(&parser, &parsed_ast);

	if (diagnostics.first) {
		diagnostics_print(&diagnostics);
		panic("Failed to parse");
	}

	for (const AstNode* node = parsed_ast.root_nodes.first; node != NULL; node = node->next) {
		if (node->kind == AST_NODE_FUNCTION) {
			if (node->function_def->body == NULL) {
				continue;
			}

			Arena input_instr_array_allocator = arena_alloc_sub_arena(context->arena, 4096);
			Arena symbol_arena = arena_alloc_sub_arena(context->arena, 1024);
			Arena strings_arena = arena_alloc_sub_arena(context->arena, 1024);

			FunctionCompiler c = {};
			c.function = node->function_def;
			c.allocator = context->arena;
			c.instr_allocator = context->arena;
			c.temp_allocator = context->temp_arena;
			c.input_instr_array_allocator = &input_instr_array_allocator;
			c.pointer_type_layout = type_layout_new(8, 8);
			c.func_ref_table.allocator = arena_allocator_new(&symbol_arena);
			c.str_storage.allocator = arena_allocator_new(&strings_arena);

			CompiledFunction func = function_compiler_compile(&c);
			compiler_resolve_default_func_refs(&func.func_ref_table);

			if (resolver) {
				resolver(&func.func_ref_table, resolver_data);
			}

			instr_replace_dead_instr(func.instr_buffer, func.usage_ranges);
			instr_print_all(func.instr_buffer, context->temp_arena);

			X64CodeGenerator gen = {};
			gen.instr_buffer = func.instr_buffer;
			gen.usage_ranges = func.usage_ranges;
			gen.allocator = context->arena;
			gen.temp_allocator = context->temp_arena;
			gen.ref_table = &func.func_ref_table;
			gen.string_consts = str_storage_to_array(&c.str_storage);

			MachineCodeBuffer machine_code = x64_generate_code(&gen, func.start_region);
			return machine_code;
		}
	}

	panic("No function to compile");
	return (MachineCodeBuffer) {};
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

	const size_t SAMPLE_COUNT = 16;

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

	const size_t SAMPLE_COUNT = 16;

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

	const size_t SAMPLE_COUNT = 16;

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

static void _resolve_symbols_for_call_inside_inner_scope(FunctionRefTable* table, void* data) {
	func_ref_table_resolve_ref_to(table, STR_LIT("store_1"), _internal_store_1);
	func_ref_table_resolve_ref_to(table, STR_LIT("store_2"), _internal_store_2);
	func_ref_table_resolve_ref_to(table, STR_LIT("store_3"), _internal_store_3);
	func_ref_table_resolve_ref_to(table, STR_LIT("store_4"), _internal_store_4);
}

void test_call_inside_inner_scope(TestContext* context) {
	String source_code = STR_LIT(
			"typedef unsigned long long uint64_t;\n"
			"void store_1(uint64_t* out);\n"
			"void store_2(uint64_t* out);\n"
			"void store_3(uint64_t* out);\n"
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
			"void store_1(uint64_t* out);\n"
			"void store_2(uint64_t* out);\n"
			"void store_3(uint64_t* out);\n"
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
			"void store_1(uint64_t* out);\n"
			"void store_2(uint64_t* out);\n"
			"void store_3(uint64_t* out);\n"
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
			"void store_1(uint64_t* out);\n"
			"void store_2(uint64_t* out);\n"
			"void store_3(uint64_t* out);\n"
			"void store_4(uint64_t* out);\n"
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
