#include "tests_x64.h"

#include "compiler/compiler.h"
#include "parser/preprocessor.h"
#include "parser/parser.h"
#include "code_gen/backends/x64.h"

#define DEFAULT_SOURCE_FILE_PATH "test.c"

typedef uint64_t(*ExecutableFunction)();

static MachineCodeBuffer _compile(TestContext* context, String source_code) {
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

			FunctionCompiler c = {};
			c.function = node->function_def;
			c.allocator = context->arena;
			c.instr_allocator = context->arena;
			c.temp_allocator = context->temp_arena;

			CompiledFunction func = function_compiler_compile(&c);

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

			x64_alloc_registers(&gen, allowed_registers);
			MachineCodeBuffer machine_code = x64_generate_code(&gen, func.start_region);
			return machine_code;
		}
	}

	panic("No function to compile");
	return (MachineCodeBuffer) {};
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
