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

	AST parsed_ast = {};
	parser_parse(&parser, &parsed_ast);

	preprocessor_release(&preprocessor);

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
		left_operand->const_32.u = 10;
	}

	{
		Instr* right_operand = instr_buffer_at(instr_buffer, right_operand_index);
		right_operand->kind = INSTR_CONST_32;
		right_operand->const_32.u = 5;
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
	InstrUsageRange* live_ranges = instr_compute_usage_ranges(*instr_buffer,
			region_index,
			context->arena,
			context->temp_arena);

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

	FunctionRefTable func_ref_table = {};
	for (size_t i = 0; i < array_size(reg_configurations); i += 1) {
		X64CodeGenerator gen = {};
		gen.flags = X64_SKIP_REG_ALLOC | X64_PRINT_SCHEDULED_IR;
		gen.instr_buffer = *instr_buffer;
		gen.usage_ranges = live_ranges;
		gen.allocator = context->arena;
		gen.temp_allocator = context->temp_arena;
		gen.ref_table = &func_ref_table;
		gen.string_consts = (StringArray) {};
		gen.instr_storage = instr_storage;

		{
			instr_storage[left_operand_index.value].kind = INSTR_STORAGE_REG;
			instr_storage[left_operand_index.value].reg = reg_configurations[i][0];

			instr_storage[right_operand_index.value].kind = INSTR_STORAGE_REG;
			instr_storage[right_operand_index.value].reg = reg_configurations[i][1];

			instr_storage[bin_op_index.value].kind = INSTR_STORAGE_REG;
			instr_storage[bin_op_index.value].reg = reg_configurations[i][2];
		}

		MachineCodeBuffer machine_code = x64_generate_code(&gen, region_index);
		
		typedef uint64_t(*Function)();

		Function function = (Function)machine_code.code;
		uint64_t result = function();

		assert(result == 5);
	}
}

static uint64_t _internal_store(uint64_t* out) {
	*out = 10;
	return 0;
}

static void _resolve_memory_operation_symbols(FunctionRefTable* table, void* data) {
	func_ref_table_resolve_ref_to(table, STR_LIT("store"), _internal_store);
}

void test_memory_operations_are_synchronized_with_calls(TestContext* context) {
	String source_code = STR_LIT(
			"typedef unsigned long long uint64_t;\n"
			"uint64_t store(uint64_t* out);\n"
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
			"uint64_t store(uint64_t* out);\n"
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

void test_encode_addressing_of_r13_and_bp(TestContext* context) {
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
	uint16_t* state = arena_alloc_array(temp_allocator, uint16_t, loc_count);
	memset(state, 0xff, sizeof(*state) * loc_count);

	// Initial state
	for (size_t i = 0; i < loc_count; i += 1) {
		assert(input_locs[i].kind == INSTR_STORAGE_REG);
		state[input_locs[i].reg] = (uint16_t)i;
	}

	// Simulate the moves
	for (size_t i = 0; i < moves.count; i += 1) {
		RegisterMove move = moves.moves[i];
		
		state[move.dst] = state[move.src];
	}

	// Assert that all the inputs are in the corresponding expected location
	bool result = true;
	for (size_t i = 0; i < loc_count; i += 1) {
		if (state[expected_locs[i]] != (uint16_t)i) {
			printf("Expected input '%u' to be at location '%u'",
					(uint32_t)i,
					(uint32_t)expected_locs[i]);
			result = false;
		}
	}

	assert(result);
}

void test_parallel_moves_is_correct_for_input_in_shifted_locations(TestContext* context) {
	X64Register expected_locs[X64_REG_COUNT];
	InstrStorageLocation input_locs[array_size(expected_locs)];

	for (size_t i = 0; i < array_size(expected_locs); i += 1) {
		expected_locs[i] = i;

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
