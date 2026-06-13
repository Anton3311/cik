#include "tests_compiler.h"

#define DEFAULT_SOURCE_FILE_PATH "test.c"

#include "compiler/compiler.h"
#include "parser/preprocessor.h"
#include "parser/parser.h"

static CompiledFunction _compile(TestContext* context, String source_code) {
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

			FunctionCompiler c = {};
			c.function = node->function_def;
			c.allocator = context->arena;
			c.instr_allocator = context->arena;
			c.temp_allocator = context->temp_arena;
			c.input_instr_array_allocator = &input_instr_array_allocator;
			c.pointer_type_layout = type_layout_new(8, 8);

			CompiledFunction func = function_compiler_compile(&c);

			instr_replace_dead_instr(func.instr_buffer, func.usage_ranges);
			instr_print_all(func.instr_buffer, context->temp_arena);

			return func;
		}
	}

	panic("No function to compile");
	return (CompiledFunction) {};
}

void test_reassigning_to_an_old_value_does_not_place_phi(TestContext* context) {
	String source_code = STR_LIT(
			"int main(int argc, char* argv[]) {\n"
			"	int a = 100;\n"
			"	if (argc == 0) {\n"
			"		int b = a;\n"
			"		a = 99;\n"
			"		a = b;\n"
			"	}\n"
			"\n"
			"	return a;\n"
			"}\n");

	CompiledFunction func = _compile(context, source_code);

	for (uint16_t i = 0; i < func.instr_buffer.count; i += 1) {
		const Instr* instr = &func.instr_buffer.instr[i];
		assert(instr->kind != INSTR_PHI);
	}
}

