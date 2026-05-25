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
			context->arena,
			context->temp_arena,
			&generated_tokens_arena);

	Arena ident_arena = { .capacity = 128 * 4096 };
	Arena ast_arena = { .capacity = 512 * 4096 };

	IdentifierStorage ident_storage = {};
	ident_storage_init(&ident_storage, heap_allocator_new(), &ident_arena);

	Parser parser = {};
	parser_init(&parser, &ast_arena, context->arena, &ident_storage, &preprocessor, &diagnostics);

	ParsedAST parsed_ast = {};
	parser_parse(&parser, &parsed_ast);

	if (diagnostics.first) {
		diagnostics_print(&diagnostics);
		panic("Failed to parse");
	}

	for (const ParsedNode* node = parsed_ast.root_nodes.first; node != NULL; node = node->next) {
		if (node->kind == AST_NODE_FUNCTION) {
			if (node->function_def->body == NULL) {
				continue;
			}

			Arena input_instr_array_allocator = arena_alloc_sub_arena(context->arena, 4096);
			Arena symbol_arena = arena_alloc_sub_arena(context->arena, 1024);

			FunctionCompiler c = {};
			c.function = node->function_def;
			c.allocator = context->arena;
			c.instr_allocator = context->arena;
			c.temp_allocator = context->temp_arena;
			c.input_instr_array_allocator = &input_instr_array_allocator;
			c.pointer_type_layout = type_layout_new(8, 8);
			c.func_ref_table.allocator = arena_allocator_new(&symbol_arena);

			CompiledFunction func = function_compiler_compile(&c);
			compiler_resolve_default_func_refs(&func.func_ref_table);

			if (resolver) {
				resolver(&func.func_ref_table, resolver_data);
			}

			instr_replace_dead_instr(func.instr_buffer, func.usage_ranges);
			instr_print_all(func.instr_buffer, context->temp_arena);

			uint16_t allowed_registers = UINT16_MAX;
			allowed_registers &= ~(1 << REG_SP);
			allowed_registers &= ~(1 << REG_BP);

			uint16_t cdecl_arg_regs[] = { REG_A, REG_C, REG_8, REG_9 };
			for (size_t i = 0; i < array_size(cdecl_arg_regs); i += 1) {
				allowed_registers &= ~(1 << cdecl_arg_regs[i]);
			}

			X64CodeGenerator gen = {};
			gen.instr_buffer = func.instr_buffer;
			gen.usage_ranges = func.usage_ranges;
			gen.allocator = context->arena;
			gen.temp_allocator = context->temp_arena;
			gen.ref_table = &func.func_ref_table;

			x64_alloc_registers(&gen, allowed_registers);
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
	free_executable(machine_code.code, machine_code.size_in_bytes);

	assert(result == 0);
}

void test_add_uint64_consts(TestContext* context) {
	String source_code = STR_LIT("unsigned long long main() { return 10 + 15; }");
	MachineCodeBuffer machine_code = _compile(context, source_code);

	ExecutableFunction executable_function = (ExecutableFunction)machine_code.code;
	uint64_t result = executable_function();
	free_executable(machine_code.code, machine_code.size_in_bytes);

	assert(result == 25);
}

void test_return_first_arg(TestContext* context) {
	String source_code = STR_LIT("unsigned long long main(unsigned long long first) { return first; }");
	MachineCodeBuffer machine_code = _compile(context, source_code);

	typedef uint64_t(*Function)(uint64_t first);

	uint64_t input = rand();

	Function executable_function = (Function)machine_code.code;
	uint64_t result = executable_function(input);
	free_executable(machine_code.code, machine_code.size_in_bytes);

	assert(result == input);
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
	free_executable(machine_code.code, machine_code.size_in_bytes);

	assert(result == a + b);
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
	free_executable(machine_code.code, machine_code.size_in_bytes);

	assert(result == value);
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
	free_executable(machine_code.code, machine_code.size_in_bytes);

	uint64_t results[16];
	for (size_t i = 0; i < array_size(array); i += 1) {
		results[i] = executable_function(array, (uint64_t)i);
	}

	for (size_t i = 0; i < array_size(array); i += 1) {
		assert(results[i] == array[i]);
	}
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
	free_executable(machine_code.code, machine_code.size_in_bytes);

	uint64_t results[16];
	for (size_t i = 0; i < array_size(array); i += 1) {
		results[i] = executable_function(array, (uint64_t)i);
	}

	for (size_t i = 0; i < array_size(array); i += 1) {
		assert(results[i] == array[i]);
	}
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
	free_executable(machine_code.code, machine_code.size_in_bytes);

	uint64_t results[SAMPLE_COUNT];
	for (size_t i = 0; i < SAMPLE_COUNT; i += 1) {
		results[i] = executable_function(array_a[i], array_b[i]);
	}

	for (size_t i = 0; i < SAMPLE_COUNT; i += 1) {
		assert(results[i] == (array_a[i] == array_b[i]));
	}
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
	free_executable(machine_code.code, machine_code.size_in_bytes);

	uint64_t results[SAMPLE_COUNT];
	for (size_t i = 0; i < SAMPLE_COUNT; i += 1) {
		results[i] = executable_function(array_a[i], array_b[i]);
	}

	for (size_t i = 0; i < SAMPLE_COUNT; i += 1) {
		assert(results[i] == (array_a[i] < array_b[i]));
	}
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
	free_executable(machine_code.code, machine_code.size_in_bytes);

	uint64_t results[SAMPLE_COUNT];
	for (size_t i = 0; i < SAMPLE_COUNT; i += 1) {
		results[i] = executable_function(array_a[i], array_b[i]);
	}

	for (size_t i = 0; i < SAMPLE_COUNT; i += 1) {
		assert(results[i] == (array_a[i] > array_b[i]));
	}
}

void test_mutate_argument(TestContext* context) {
	String source_code = STR_LIT(
			"typedef unsigned long long uint64;\n"
			"uint64 main(uint64 a) { a = 100; return a; }");
	MachineCodeBuffer machine_code = _compile(context, source_code);

	typedef uint64_t(*Function)(uint64_t a);

	Function executable_function = (Function)machine_code.code;
	free_executable(machine_code.code, machine_code.size_in_bytes);

	uint64_t result = executable_function(101);

	assert(result == 100);
}
