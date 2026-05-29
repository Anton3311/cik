#include "tests.h"

#include "parser/preprocessor.h"
#include "parser/parser.h"

#define DEFAULT_SOURCE_PATH "test.c"

//
// Source Info
//

void test_text_start_position_to_source_location(TestContext* context) {
	String source_code = STR_LIT("#define hello 100 + 100\nhello");

	LineInfo line_info = line_info_from_source(context->arena, source_code);

	SourceLocation position = line_info_pos_to_source_location(&line_info, 0);
	assert(position.line == 0);
	assert(position.column == 0);
}

void test_last_line_postion_to_source_location(TestContext* context) {
	String source_code = STR_LIT("#define hello 100 + 100\nhello");

	LineInfo line_info = line_info_from_source(context->arena, source_code);

	SourceLocation position = line_info_pos_to_source_location(&line_info, 26);
	assert(position.line == 1);
	assert(position.column == 2);
}

void test_line_range_of_one_line_source_code(TestContext* context) {
	String source_code = STR_LIT("hello");

	LineInfo line_info = line_info_from_source(context->arena, source_code);

	SourceRange range = line_info_get_line_range(&line_info, 0);
	assert(range.start == 0);
	assert(range.end == 5);
}

//
// Tokenizer
//

void test_token_has_valid_string_represenation(TestContext* context) {
	for (size_t i = 0; i < TOKEN_COUNT; i += 1) {
		String string = token_kind_to_string((TokenKind)i);
		assert(string.v != NULL);
		assert(string.length > 0);
	}
}

typedef struct {
	size_t token_count;
	TokenKind* generated_tokens;
	String generated_source;
} RandomTokenStream;

RandomTokenStream _generate_random_tokens(TestContext* context, size_t token_count) {
	size_t acceptable_token_count = TOKEN_COUNT - 1; // - 1 because TOKEN_EOF is exluded
	TokenKind* generated_tokens = arena_alloc_array(context->arena, TokenKind, token_count);

	StringBuilder builder = { .arena = context->temp_arena };
	for (size_t i = 0; i < token_count; i += 1) {
		size_t token_index = (size_t)rand() % acceptable_token_count;
		
		TokenKind token_kind = (TokenKind)(1 + token_index);
		assert(token_kind != TOKEN_EOF);

		generated_tokens[i] = token_kind;

		String token_string = {};
		switch (token_kind) {
		case TOKEN_IDENT:
			token_string = STR_LIT("ident");
			break;
		case TOKEN_STRING:
			token_string = STR_LIT("\"hello world\"");
			break;
		default:
			token_string = token_kind_to_string(token_kind);
			if (token_string.length >= 2) {
				assert_msg(
						token_string.v[0] != '<' || token_string.v[token_string.length - 1] != '>',
						"Some tokens that don't have an explicit string representation were not handled");
			}
		}

		str_builder_append(&builder, token_string);
		str_builder_append_char(&builder, '\n');
	}

	return (RandomTokenStream) {
		.token_count = token_count,
		.generated_tokens = generated_tokens,
		.generated_source = builder.string,
	};
}

void test_token_source_range_matches_token_string(TestContext* context) {
	assert(TOKEN_EOF == 0);

	size_t token_count = 4096;
	RandomTokenStream random_tokens = _generate_random_tokens(context, token_count);

	Tokenizer tokenizer = {
		.source_code = random_tokens.generated_source,
	};

	size_t generated_token_count = 0;
	while (true) {
		Token token = tokenizer_next_token(&tokenizer);

		if (token.kind == TOKEN_EOF) {
			break;
		}

		generated_token_count += 1;
		assert(generated_token_count <= token_count);

		String source_sub_str = sub_str(random_tokens.generated_source,
				token.source_range.start,
				token.source_range.end - token.source_range.start);

		assert(str_equal(source_sub_str, token.string));
	}
}

void test_tokenizer_generates_expected_token(TestContext* context) {
	assert(TOKEN_EOF == 0);

	size_t token_count = 4096;
	RandomTokenStream random_tokens = _generate_random_tokens(context, token_count);

	Tokenizer tokenizer = {
		.source_code = random_tokens.generated_source,
	};

	size_t generated_token_count = 0;
	while (true) {
		Token token = tokenizer_next_token(&tokenizer);

		if (token.kind == TOKEN_EOF) {
			break;
		}

		TokenKind expected_kind = random_tokens.generated_tokens[generated_token_count];
		assert_msg(token.kind == expected_kind,
				"Expected: '%.*s' Actual: '%.*s'",
				STR_FMT(token_kind_to_string(expected_kind)),
				STR_FMT(token_kind_to_string(token.kind)));

		generated_token_count += 1;
		assert(generated_token_count <= token_count);
	}
}

void test_mutli_line_comment_with_asterisk_on_line_starts(TestContext* context) {
	String source_code = STR_LIT(
			"/* start of multi line comment\n"
			" * line starting with asterisk\n"
			" * another line starting with asterisk\n"
			" */");

	Tokenizer tokenizer = {
		.source_code = source_code,
	};

	Token token = tokenizer_next_token(&tokenizer);
	assert(token.kind == TOKEN_EOF);
}

void test_token_has_valid_source_file(TestContext* context) {
	size_t token_count = 4096;
	RandomTokenStream random_tokens = _generate_random_tokens(context, token_count);

	SourceFile source_file = {
		.path = STR_LIT(DEFAULT_SOURCE_PATH),
		.source_code = random_tokens.generated_source
	};

	Tokenizer tokenizer = {};
	tokenizer_init(&tokenizer, &source_file);

	while (true) {
		Token token = tokenizer_next_token(&tokenizer);

		if (token.kind == TOKEN_EOF) {
			break;
		}

		assert(token.source_range.source_file != NULL);
	}
}

//
// Preprocessor
//

static void init_preprocessor_test(TestContext* context,
		String source_code,
		Preprocessor* out_preprocessor,
		Diagnostics* out_diagnostics,
		LineInfo* out_line_info) {

	SourceStorage* source_storage = arena_alloc(context->arena, SourceStorage);
	source_storage_init(source_storage, (StringArray) {}, context->arena);
	SourceFile* source_file = source_storage_append(source_storage, STR_LIT(DEFAULT_SOURCE_PATH), source_code);

	Arena* generated_tokens_arena = arena_alloc(context->arena, Arena);
	*generated_tokens_arena = arena_alloc_sub_arena(context->arena, 2 * 4096);

	*out_diagnostics = (Diagnostics) {
		.allocator = context->arena,
	};
	
	preprocessor_init(out_preprocessor,
			source_storage,
			source_file,
			out_diagnostics,
			context->arena,
			context->temp_arena,
			generated_tokens_arena);
}

void test_non_function_style_macro_expansion(TestContext* context) {
	String source_code = STR_LIT(
			"#define hello 100 + 100\n"
			"hello");
	String expected_source_code = STR_LIT("100 + 100");

	LineInfo line_info = {};
	Diagnostics diagnostics = {};
	Preprocessor preprocessor = {};

	init_preprocessor_test(context, source_code, &preprocessor, &diagnostics, &line_info);

	Tokenizer expected_source_tokenizer = (Tokenizer) {
		.source_code = expected_source_code,
	};

	while (true) {
		Token token = preprocessor_next_token(&preprocessor);
		Token expected_token = tokenizer_next_token(&expected_source_tokenizer);

		assert(token.kind == expected_token.kind);
		assert(str_equal(token.string, expected_token.string));

		if (token.kind == TOKEN_EOF) {
			break;
		}
	}
}

void test_expand_function_style_macro_with_two_params(TestContext* context) {
	String source_code = STR_LIT(
			"#define hello(a, b) a + b\n"
			"hello(10, 10)");
	String expected_source_code = STR_LIT("10 + 10");

	LineInfo line_info = {};
	Diagnostics diagnostics = {};
	Preprocessor preprocessor = {};

	init_preprocessor_test(context, source_code, &preprocessor, &diagnostics, &line_info);

	Tokenizer expected_source_tokenizer = (Tokenizer) {
		.source_code = expected_source_code,
	};

	while (true) {
		Token token = preprocessor_next_token(&preprocessor);
		Token expected_token = tokenizer_next_token(&expected_source_tokenizer);

		assert(token.kind == expected_token.kind);
		assert(str_equal(token.string, expected_token.string));

		if (token.kind == TOKEN_EOF) {
			break;
		}
	}
}

void test_expand_function_style_macro_without_params(TestContext* context) {
	String source_code = STR_LIT(
			"#define hello() token\n"
			"hello()");
	String expected_source_code = STR_LIT("token");

	LineInfo line_info = {};
	Diagnostics diagnostics = {};
	Preprocessor preprocessor = {};

	init_preprocessor_test(context, source_code, &preprocessor, &diagnostics, &line_info);

	Tokenizer expected_source_tokenizer = (Tokenizer) {
		.source_code = expected_source_code,
	};

	while (true) {
		Token token = preprocessor_next_token(&preprocessor);
		Token expected_token = tokenizer_next_token(&expected_source_tokenizer);

		assert(token.kind == expected_token.kind);
		assert(str_equal(token.string, expected_token.string));

		if (token.kind == TOKEN_EOF) {
			break;
		}
	}
}

void test_expand_empty_function_style_macro(TestContext* context) {
	String source_code = STR_LIT(
			"#define ignore(a)\n"
			"ignore(10)");

	LineInfo line_info = {};
	Diagnostics diagnostics = {};
	Preprocessor preprocessor = {};

	init_preprocessor_test(context, source_code, &preprocessor, &diagnostics, &line_info);

	Token token = preprocessor_next_token(&preprocessor);
	assert(token.kind == TOKEN_EOF);
}

void test_expand_empty_style_macro(TestContext* context) {
	String source_code = STR_LIT(
			"#define empty\n"
			"empty");

	LineInfo line_info = {};
	Diagnostics diagnostics = {};
	Preprocessor preprocessor = {};

	init_preprocessor_test(context, source_code, &preprocessor, &diagnostics, &line_info);

	Token token = preprocessor_next_token(&preprocessor);
	assert(token.kind == TOKEN_EOF);
}

void test_macro_call_with_not_enough_args_fails(TestContext* context) {
	String source_code = STR_LIT(
			"#define many_args(a, b, c, d) a + b + c + d\n"
			"many_args(10, 1)");

	LineInfo line_info = {};
	Diagnostics diagnostics = {};
	Preprocessor preprocessor = {};

	init_preprocessor_test(context, source_code, &preprocessor, &diagnostics, &line_info);

	preprocessor_next_token(&preprocessor);

	assert(diagnostics.first != NULL);

	String expected_error_message = STR_LIT("Not enough arguments during a call of macro called 'many_args'. "
			"Expected 4 but only 2 were provided.");

	assert(str_equal(diagnostics.first->message, expected_error_message));
}

void test_nested_macro(TestContext* context) {
	String source_code = STR_LIT(
			"#define inner(a, b) a + b\n"
			"#define outer(a, b, c) (inner(a, b)) * c\n"
			"outer(1, 2, 3)");

	String expected_source_code = STR_LIT("(1 + 2) * 3");

	LineInfo line_info = {};
	Diagnostics diagnostics = {};
	Preprocessor preprocessor = {};

	init_preprocessor_test(context, source_code, &preprocessor, &diagnostics, &line_info);

	Tokenizer expected_source_tokenizer = (Tokenizer) {
		.source_code = expected_source_code,
	};

	while (true) {
		Token token = preprocessor_next_token(&preprocessor);
		Token expected_token = tokenizer_next_token(&expected_source_tokenizer);

		printf("%.*s %.*s\n", STR_FMT(token.string), STR_FMT(expected_token.string));

		assert(token.kind == expected_token.kind);
		assert(str_equal(token.string, expected_token.string));

		if (token.kind == TOKEN_EOF) {
			break;
		}
	}
}

void test_nested_macro_2(TestContext* context) {
	String source_code = STR_LIT(
			"#define inner(a, b) a + b\n"
			"#define inner2(a, b) inner(a, b) + b\n"
			"#define outer(a, b, c) (inner2(a, b) + inner2(b, a)) * c\n"
			"outer(1, 2, 3)");

	String expected_source_code = STR_LIT("(1 + 2 + 2 + 2 + 1 + 1) * 3");

	LineInfo line_info = {};
	Diagnostics diagnostics = {};
	Preprocessor preprocessor = {};

	init_preprocessor_test(context, source_code, &preprocessor, &diagnostics, &line_info);

	Tokenizer expected_source_tokenizer = (Tokenizer) {
		.source_code = expected_source_code,
	};

	while (true) {
		Token token = preprocessor_next_token(&preprocessor);
		Token expected_token = tokenizer_next_token(&expected_source_tokenizer);

		printf("%.*s %.*s\n", STR_FMT(token.string), STR_FMT(expected_token.string));

		assert(token.kind == expected_token.kind);
		assert(str_equal(token.string, expected_token.string));

		if (token.kind == TOKEN_EOF) {
			break;
		}
	}
}

void test_builtin_line_macro_expantion(TestContext* context) {
	String source_code = STR_LIT(
		"__LINE__\n" // 1
		"__LINE__\n" // 2
		"__LINE__\n" // 3
		"__LINE__ __LINE__ __LINE__" // 4 4 4
	);

	String expected_source_code = STR_LIT("1 2 3 4 4 4");

	LineInfo line_info = {};
	Diagnostics diagnostics = {};
	Preprocessor preprocessor = {};

	init_preprocessor_test(context, source_code, &preprocessor, &diagnostics, &line_info);

	Tokenizer expected_source_tokenizer = (Tokenizer) {
		.source_code = expected_source_code,
	};

	while (true) {
		Token token = preprocessor_next_token(&preprocessor);
		Token expected_token = tokenizer_next_token(&expected_source_tokenizer);

		printf("%.*s %.*s\n", STR_FMT(token.string), STR_FMT(expected_token.string));

		assert(token.kind == expected_token.kind);
		assert(str_equal(token.string, expected_token.string));

		if (token.kind == TOKEN_EOF) {
			break;
		}
	}
}

void test_assert_macro_expantion(TestContext* context) {
	String source_code = STR_LIT(
		"#define assert(expression) if (!(expression)) { printf(\"%s:%u\", __FILE__, __LINE__); }\n"
		"assert(true)"
	);

	String expected_source_code = STR_LIT("if (!(true)) { printf(\"%s:%u\", \"" DEFAULT_SOURCE_PATH "\", 2); }");

	LineInfo line_info = {};
	Diagnostics diagnostics = {};
	Preprocessor preprocessor = {};

	init_preprocessor_test(context, source_code, &preprocessor, &diagnostics, &line_info);

	Tokenizer expected_source_tokenizer = (Tokenizer) {
		.source_code = expected_source_code,
	};

	while (true) {
		Token token = preprocessor_next_token(&preprocessor);
		Token expected_token = tokenizer_next_token(&expected_source_tokenizer);

		printf("%.*s %.*s\n", STR_FMT(token.string), STR_FMT(expected_token.string));

		assert(token.kind == expected_token.kind);
		assert(str_equal(token.string, expected_token.string));

		if (token.kind == TOKEN_EOF) {
			break;
		}
	}
}

void test_macro_string_operator(TestContext* context) {
	String source_code = STR_LIT(
		"#define to_string(a) #a\n"
		"to_string(hello world)"
	);

	String expected_source_code = STR_LIT("\"hello world\"");

	LineInfo line_info = {};
	Diagnostics diagnostics = {};
	Preprocessor preprocessor = {};

	init_preprocessor_test(context, source_code, &preprocessor, &diagnostics, &line_info);

	Tokenizer expected_source_tokenizer = (Tokenizer) {
		.source_code = expected_source_code,
	};

	while (true) {
		Token token = preprocessor_next_token(&preprocessor);
		Token expected_token = tokenizer_next_token(&expected_source_tokenizer);

		printf("%.*s %.*s\n", STR_FMT(token.string), STR_FMT(expected_token.string));

		assert(token.kind == expected_token.kind);
		assert(str_equal(token.string, expected_token.string));

		if (token.kind == TOKEN_EOF) {
			break;
		}
	}
}

void test_macro_string_operator_with_invalid_param_name_fails(TestContext* context) {
	String source_code = STR_LIT(
		"#define to_string(a) #invalid_param\n"
		"to_string(hello world)"
	);

	LineInfo line_info = {};
	Diagnostics diagnostics = {};
	Preprocessor preprocessor = {};

	init_preprocessor_test(context, source_code, &preprocessor, &diagnostics, &line_info);

	while (true) {
		Token token = preprocessor_next_token(&preprocessor);
		if (token.kind == TOKEN_EOF) {
			break;
		}
	}
	
	assert(diagnostics.first != NULL);

	// NOTE: Line indices are zero based
	assert(diagnostics.first->start_line + 1 == 1);
	assert(diagnostics.first->end_line + 1 == 1);
}

void test_simple_if_elif_directives(TestContext* context) {
	String source_code = STR_LIT(
			"#if 0\n"
			"hello\n"
			"#elif 1\n"
			"world\n"
			"#endif\n"
	);

	LineInfo line_info = {};
	Diagnostics diagnostics = {};
	Preprocessor preprocessor = {};

	init_preprocessor_test(context, source_code, &preprocessor, &diagnostics, &line_info);

	Token* tokens = arena_alloc_array(context->temp_arena, Token, 0);
	size_t token_count = 0;

	while (true) {
		Token token = preprocessor_next_token(&preprocessor);
		if (token.kind == TOKEN_EOF) {
			break;
		}

		*arena_alloc(context->temp_arena, Token) = token;
		token_count += 1;
	}

	assert(token_count == 1);
	assert(tokens[0].kind == TOKEN_IDENT);
	assert(str_equal(tokens[0].string, STR_LIT("world")));
}

void test_parsing_of_non_function_style_macro_with_paren_as_first_token_in_stream(TestContext* context) {
	String source_code = STR_LIT(
		"#define macro  (token)\n"
		"macro"
	);

	String expected_source_code = STR_LIT("(token)");

	LineInfo line_info = {};
	Diagnostics diagnostics = {};
	Preprocessor preprocessor = {};

	init_preprocessor_test(context, source_code, &preprocessor, &diagnostics, &line_info);

	Tokenizer expected_source_tokenizer = (Tokenizer) {
		.source_code = expected_source_code,
	};

	while (true) {
		Token token = preprocessor_next_token(&preprocessor);
		Token expected_token = tokenizer_next_token(&expected_source_tokenizer);

		printf("%.*s %.*s\n", STR_FMT(token.string), STR_FMT(expected_token.string));

		assert(token.kind == expected_token.kind);
		assert(str_equal(token.string, expected_token.string));

		if (token.kind == TOKEN_EOF) {
			break;
		}
	}
}

void test_error_directive(TestContext* context) {
	String source_code = STR_LIT(
		"#error Error message"
	);

	LineInfo line_info = {};
	Diagnostics diagnostics = {};
	Preprocessor preprocessor = {};

	init_preprocessor_test(context, source_code, &preprocessor, &diagnostics, &line_info);

	while (preprocessor_next_token(&preprocessor).kind != TOKEN_EOF);

	assert(diagnostics.first != NULL);
	assert(str_equal(diagnostics.first->message, STR_LIT("Error message")));
}

void test_multi_line_define(TestContext* context) {
	String source_code = STR_LIT(
		"#define macro hello\\\n"
		"world\n"
		"macro"
	);

	String expected_source_code = STR_LIT("hello\nworld");

	LineInfo line_info = {};
	Diagnostics diagnostics = {};
	Preprocessor preprocessor = {};

	init_preprocessor_test(context, source_code, &preprocessor, &diagnostics, &line_info);

	Tokenizer expected_source_tokenizer = (Tokenizer) {
		.source_code = expected_source_code,
	};

	while (true) {
		Token token = preprocessor_next_token(&preprocessor);
		Token expected_token = tokenizer_next_token(&expected_source_tokenizer);

		printf("%.*s %.*s\n", STR_FMT(token.string), STR_FMT(expected_token.string));

		assert(token.kind == expected_token.kind);
		assert(str_equal(token.string, expected_token.string));

		if (token.kind == TOKEN_EOF) {
			break;
		}
	}
}

void test_token_insertion_operator(TestContext* context) {
	String source_code = STR_LIT(
		"#define macro(a) hello##a\n"
		"macro(_world)"
	);

	String expected_source_code = STR_LIT("hello_world");

	LineInfo line_info = {};
	Diagnostics diagnostics = {};
	Preprocessor preprocessor = {};

	init_preprocessor_test(context, source_code, &preprocessor, &diagnostics, &line_info);

	Tokenizer expected_source_tokenizer = (Tokenizer) {
		.source_code = expected_source_code,
	};

	while (true) {
		Token token = preprocessor_next_token(&preprocessor);
		Token expected_token = tokenizer_next_token(&expected_source_tokenizer);

		printf("%.*s %.*s\n", STR_FMT(token.string), STR_FMT(expected_token.string));

		assert(token.kind == expected_token.kind);
		assert(str_equal(token.string, expected_token.string));

		if (token.kind == TOKEN_EOF) {
			break;
		}
	}
}

void test_va_args_macro(TestContext* context) {
	String source_code = STR_LIT(
		"#define macro(...) __VA_ARGS__\n"
		"macro(10, 11, 88)"
	);

	String expected_source_code = STR_LIT("10, 11, 88");

	LineInfo line_info = {};
	Diagnostics diagnostics = {};
	Preprocessor preprocessor = {};

	init_preprocessor_test(context, source_code, &preprocessor, &diagnostics, &line_info);

	Tokenizer expected_source_tokenizer = (Tokenizer) {
		.source_code = expected_source_code,
	};

	while (true) {
		Token token = preprocessor_next_token(&preprocessor);
		Token expected_token = tokenizer_next_token(&expected_source_tokenizer);

		printf("%.*s %.*s\n", STR_FMT(token.string), STR_FMT(expected_token.string));

		assert(token.kind == expected_token.kind);
		assert(str_equal(token.string, expected_token.string));

		if (token.kind == TOKEN_EOF) {
			break;
		}
	}
}

void test_processor_next_returns_eof_when_include_stack_is_empty(TestContext* context) {
	String source_code = STR_LIT("hello");

	LineInfo line_info = {};
	Diagnostics diagnostics = {};
	Preprocessor preprocessor = {};

	init_preprocessor_test(context, source_code, &preprocessor, &diagnostics, &line_info);

	Token first_token = preprocessor_next_token(&preprocessor);
	assert(first_token.kind == TOKEN_IDENT);
	Token second_token = preprocessor_next_token(&preprocessor);
	assert(second_token.kind == TOKEN_EOF);

	assert_msg(preprocessor.include_stack.depth == 0,
			"Preprocessor has reached the eof and should have popped the "
			"file off of the include_stack");

	Token third_token = preprocessor_next_token(&preprocessor);
	assert_msg(third_token.kind == TOKEN_EOF,
			"Preprocessor has reached the eof and has no more files in the include stack, "
			"thus must keep on return the eof");
}

//
// Parser
//

void run_parser_test_2(TestContext* context,
		Diagnostics* out_diagnostics,
		SourceStorage* out_source_storage,
		String source_code,
		ParsedAST* out_ast,
		IdentifierStorage** out_ident_storage) {

	source_storage_init(out_source_storage, (StringArray) {}, context->arena);
	SourceFile* source_file = source_storage_append(out_source_storage, STR_LIT(DEFAULT_SOURCE_PATH), source_code);

	*out_diagnostics = (Diagnostics) {
		.allocator = context->temp_arena,
	};

	Arena generated_tokens_arena = arena_alloc_sub_arena(context->arena, 2 * 4096);

	Preprocessor preprocessor = {};
	preprocessor_init(&preprocessor,
			out_source_storage,
			source_file,
			out_diagnostics,
			context->arena,
			context->temp_arena,
			&generated_tokens_arena);

	*out_ident_storage = arena_alloc(context->arena, IdentifierStorage);

	// NOTE: This arena is used by the `IdentifierStorage` inside the parser,
	//       so it's lifetime is no longer than the one of the parser.
	Arena ident_arena = arena_alloc_sub_arena(context->arena, 16 * 1024);
	Arena ast_arena = arena_alloc_sub_arena(context->arena, 16 * 1024);

	ident_storage_init(*out_ident_storage, arena_allocator_new(&ident_arena), &ident_arena);

	Parser parser = {};
	parser_init(&parser, &ast_arena, context->temp_arena, *out_ident_storage, &preprocessor, out_diagnostics);

	parser_parse(&parser, out_ast);
}

void run_parser_test(TestContext* context,
		Diagnostics* out_diagnostics,
		SourceStorage* out_source_storage,
		String source_code,
		ParsedAST* out_ast) {
	IdentifierStorage* ident_storage;
	run_parser_test_2(context,
			out_diagnostics,
			out_source_storage,
			source_code,
			out_ast,
			&ident_storage);
}

void test_parse_type_def_of_primitive_type(TestContext* context) {
	SourceStorage source_storage;
	Diagnostics diagnostics;
	ParsedAST ast;
	run_parser_test(context, &diagnostics, &source_storage, STR_LIT("typedef int int32;"), &ast);

	assert(diagnostics.first == NULL);

	assert(ast.root_nodes.count == 1);

	ParsedNode* first = ast.root_nodes.first;
	assert(first->kind == AST_NODE_TYPE_DEF);

	ParsedTypeDef* type_def = first->type_def;
	assert(type_def->aliased_type.kind == PARSED_TYPE_INT);
	assert(str_equal(type_def->new_name.string, STR_LIT("int32")));
}

void test_parse_type_def_of_struct_def(TestContext* context) {
	SourceStorage source_storage;
	Diagnostics diagnostics;
	ParsedAST ast;
	run_parser_test(context, &diagnostics, &source_storage, STR_LIT("typedef struct Hello World;"), &ast);

	assert(diagnostics.first == NULL);
	assert(ast.root_nodes.count == 1);

	ParsedNode* first = ast.root_nodes.first;
	assert(first->kind == AST_NODE_TYPE_DEF);

	ParsedTypeDef* type_def = first->type_def;
	assert(type_def->aliased_type.kind == PARSED_TYPE_STRUCT);

	ParsedStruct* struct_def = type_def->aliased_type.struct_def;
	assert(str_equal(struct_def->name.string, STR_LIT("Hello")));
	assert(struct_def->field_count == 0);

	assert(str_equal(type_def->new_name.string, STR_LIT("World")));
}

void test_parse_type_def_of_struct_def_with_fields(TestContext* context) {
	SourceStorage source_storage;
	Diagnostics diagnostics;
	ParsedAST ast;
	run_parser_test(context, &diagnostics, &source_storage, STR_LIT("typedef struct Hello {\n"
				"	int int_value;\n"
				"	float float_value;\n"
				"	struct InnerStruct { int inner_value; } inner;\n"
				"} World;"), &ast);

	assert(diagnostics.first == NULL);
	assert(ast.root_nodes.count == 1);

	ParsedNode* first = ast.root_nodes.first;
	assert(first->kind == AST_NODE_TYPE_DEF);

	ParsedTypeDef* type_def = first->type_def;
	assert(type_def->aliased_type.kind == PARSED_TYPE_STRUCT);
	assert(str_equal(type_def->new_name.string, STR_LIT("World")));

	ParsedStruct* hello_struct_def = type_def->aliased_type.struct_def;
	assert(str_equal(hello_struct_def->name.string, STR_LIT("Hello")));
	assert(hello_struct_def->field_count == 3);

	ParsedStructField* int_value_field = &hello_struct_def->fields[0];
	ParsedStructField* float_value_field = &hello_struct_def->fields[1];
	ParsedStructField* inner_field = &hello_struct_def->fields[2];
	
	assert(int_value_field->type.kind == PARSED_TYPE_INT);
	assert(str_equal(int_value_field->name.string, STR_LIT("int_value")));

	assert(float_value_field->type.kind == PARSED_TYPE_FLOAT);
	assert(str_equal(float_value_field->name.string, STR_LIT("float_value")));

	// Check InnerStruct
	assert(inner_field->type.kind == PARSED_TYPE_STRUCT);

	ParsedStruct* inner_struct_def = inner_field->type.struct_def;
	assert(str_equal(inner_struct_def->name.string, STR_LIT("InnerStruct")));

	ParsedStructField* inner_value_field = inner_struct_def->fields;
	assert(inner_value_field->type.kind == PARSED_TYPE_INT);
	assert(str_equal(inner_value_field->name.string, STR_LIT("inner_value")));
}

void test_aliased_type_resolution(TestContext* context) {
	SourceStorage source_storage;
	Diagnostics diagnostics;
	ParsedAST ast;
	run_parser_test(context, &diagnostics, &source_storage, STR_LIT("typedef int int32;\n"
				"int32 number;"), &ast);

	assert(diagnostics.first == NULL);
	assert(ast.root_nodes.count == 2);

	ParsedNode* type_def_node = ast.root_nodes.first;
	ParsedNode* var_node = type_def_node->next;

	assert(type_def_node->kind == AST_NODE_TYPE_DEF);
	assert(var_node->kind == AST_NODE_VARIABLE);

	assert(var_node->variable.type.alias_definition == type_def_node->type_def);
}

void test_parse_enum_def(TestContext* context) {
	SourceStorage source_storage;
	Diagnostics diagnostics;
	ParsedAST ast;
	run_parser_test(context, &diagnostics, &source_storage, STR_LIT("enum Type {\n"
				"	TYPE_INT,\n"
				"	TYPE_FLOAT\n"
				"};"), &ast);

	diagnostics_print(&diagnostics);
	assert(diagnostics.first == NULL);
	assert(ast.root_nodes.count == 1);

	ParsedNode* first = ast.root_nodes.first;
	assert(first->kind == AST_NODE_ENUM);

	ParsedEnum* enum_def = first->enum_def;
	assert(enum_def != NULL);
	assert(str_equal(enum_def->name.string, STR_LIT("Type")));
	assert(enum_def->variant_count == 2);

	ParsedEnumVariant* first_variant = &enum_def->variants[0];
	ParsedEnumVariant* second_variant = &enum_def->variants[1];

	assert(str_equal(first_variant->name.string, STR_LIT("TYPE_INT")));
	assert(str_equal(second_variant->name.string, STR_LIT("TYPE_FLOAT")));
}

void test_parse_function_def(TestContext* context) {
	SourceStorage source_storage;
	Diagnostics diagnostics;
	ParsedAST ast;
	run_parser_test(context, &diagnostics, &source_storage, STR_LIT("void func(int a, int b);"), &ast);

	diagnostics_print(&diagnostics);
	assert(diagnostics.first == NULL);
	assert(ast.root_nodes.count == 1);

	ParsedNode* first = ast.root_nodes.first;
	assert(first->kind == AST_NODE_FUNCTION);

	ParsedFunction* function_def = first->function_def;
	assert(str_equal(function_def->name.string, STR_LIT("func")));

	ParsedType* return_type = &function_def->return_type;
	assert(return_type->kind == PARSED_TYPE_VOID);

	assert(function_def->parameter_count == 2);
	const ParsedFunctionParam* first_param = &function_def->parameters[0];
	const ParsedFunctionParam* second_param = &function_def->parameters[1];

	assert(str_equal(first_param->name.string, STR_LIT("a")));
	assert(first_param->type.kind == PARSED_TYPE_INT);

	assert(str_equal(second_param->name.string, STR_LIT("b")));
	assert(second_param->type.kind == PARSED_TYPE_INT);
}

void test_parse_forward_declared_struct(TestContext* context) {
	SourceStorage source_storage;
	Diagnostics diagnostics;
	ParsedAST ast;
	run_parser_test(context, &diagnostics, &source_storage, STR_LIT("struct Hello;"), &ast);

	diagnostics_print(&diagnostics);
	assert(diagnostics.first == NULL);
	assert(ast.root_nodes.count == 1);

	ParsedNode* first_def = ast.root_nodes.first;
	assert(first_def->kind == AST_NODE_STRUCT);

	assert(first_def->struct_def->is_forward_declared);
}

void test_parse_forward_declared_struct_followed_by_definition(TestContext* context) {
	SourceStorage source_storage;
	Diagnostics diagnostics;
	ParsedAST ast;
	run_parser_test(context, &diagnostics, &source_storage, STR_LIT("struct Hello; struct Hello {};"), &ast);

	diagnostics_print(&diagnostics);
	assert(diagnostics.first == NULL);
	assert(ast.root_nodes.count == 2);

	ParsedNode* first_def = ast.root_nodes.first;
	assert(first_def->kind == AST_NODE_STRUCT);
	assert(first_def->next != NULL);

	ParsedNode* second_def = first_def->next;
	assert(second_def->kind == AST_NODE_STRUCT);

	assert(!first_def->struct_def->is_forward_declared);
	assert(first_def->struct_def == second_def->struct_def);
}

void test_parse_forward_declared_enum(TestContext* context) {
	SourceStorage source_storage;
	Diagnostics diagnostics;
	ParsedAST ast;
	run_parser_test(context, &diagnostics, &source_storage, STR_LIT("enum Hello;"), &ast);

	diagnostics_print(&diagnostics);
	assert(diagnostics.first == NULL);
	assert(ast.root_nodes.count == 1);

	ParsedNode* first_def = ast.root_nodes.first;
	assert(first_def->kind == AST_NODE_ENUM);

	assert(first_def->enum_def->is_forward_declared);
}

void test_parse_forward_declared_enum_followed_by_definition(TestContext* context) {
	SourceStorage source_storage;
	Diagnostics diagnostics;
	ParsedAST ast;
	run_parser_test(context, &diagnostics, &source_storage, STR_LIT("enum Hello; enum Hello {};"), &ast);

	diagnostics_print(&diagnostics);
	assert(diagnostics.first == NULL);
	assert(ast.root_nodes.count == 2);

	ParsedNode* first_def = ast.root_nodes.first;
	assert(first_def->kind == AST_NODE_ENUM);
	assert(first_def->next != NULL);

	ParsedNode* second_def = first_def->next;
	assert(second_def->kind == AST_NODE_ENUM);

	assert(!first_def->enum_def->is_forward_declared);
	assert(first_def->enum_def == second_def->enum_def);
}

void test_parse_forward_declared_function(TestContext* context) {
	SourceStorage source_storage;
	Diagnostics diagnostics;
	ParsedAST ast;
	run_parser_test(context, &diagnostics, &source_storage, STR_LIT("int add(int a, int b);"), &ast);

	diagnostics_print(&diagnostics);
	assert(diagnostics.first == NULL);
	assert(ast.root_nodes.count == 1);

	ParsedNode* first_def = ast.root_nodes.first;
	assert(first_def->kind == AST_NODE_FUNCTION);
	assert(first_def->function_def->is_forward_declared);
	assert(first_def->function_def->body == NULL);
}

void test_parse_forward_declared_function_followed_by_definition(TestContext* context) {
	SourceStorage source_storage;
	Diagnostics diagnostics;
	ParsedAST ast;
	run_parser_test(context,
			&diagnostics,
			&source_storage,
			STR_LIT(
				"int add(int a, int b);\n"
				"int add(int a, int b) {}"),
			&ast);

	diagnostics_print(&diagnostics);
	assert(diagnostics.first == NULL);
	assert(ast.root_nodes.count == 2);

	ParsedNode* first_def = ast.root_nodes.first;
	assert(first_def->kind == AST_NODE_FUNCTION);
	assert(first_def->next != NULL);

	ParsedNode* second_def = first_def->next;
	assert(second_def->kind == AST_NODE_FUNCTION);

	assert(!first_def->function_def->is_forward_declared);
	assert(first_def->function_def == second_def->function_def);
	assert(first_def->function_def->body != NULL);
}

void test_parse_function_ref_expr(TestContext* context) {
	SourceStorage source_storage;
	Diagnostics diagnostics;
	ParsedAST ast;
	run_parser_test(context, &diagnostics, &source_storage,
			STR_LIT(
				"void add(int a, int b);"
				"void add(int a, int b) { add; }"), &ast);

	diagnostics_print(&diagnostics);
	assert(diagnostics.first == NULL);
	assert(ast.root_nodes.count == 2);

	ParsedNode* first_def = ast.root_nodes.first;
	assert(first_def->kind == AST_NODE_FUNCTION);
	assert(first_def->function_def->body != NULL);
	assert(first_def->function_def->body->nodes.count == 1);

	ParsedNode* body_node = first_def->function_def->body->nodes.first;
	assert(body_node->kind == AST_NODE_EXPR);
	assert(body_node->expr.kind == EXPR_FUNCTION_REFERENCE);
	assert(body_node->expr.function_ref == first_def->function_def);
}

void test_parse_primitive_integer_types(TestContext* context) {
	StringBuilder builder = { .arena = context->temp_arena };

	ParsedTypeKind type_kinds[] = {
		PARSED_TYPE_CHAR,
		PARSED_TYPE_INT,
		PARSED_TYPE_SHORT,
		PARSED_TYPE_LONG,
		PARSED_TYPE_LONG_LONG
	};

	String type_kind_names[] = {
		STR_LIT("char"),
		STR_LIT("int"),
		STR_LIT("short"),
		STR_LIT("long"),
		STR_LIT("long long"),
	};

	ParsedTypeKindFlags type_flags[] = {
		TYPE_FLAG_NONE,
		TYPE_FLAG_SIGNED,
		TYPE_FLAG_UNSIGNED,
	};

	String type_flag_names[] = {
		STR_LIT(""),
		STR_LIT("signed"),
		STR_LIT("unsigned")
	};

	str_builder_append(&builder, STR_LIT("struct Types {\n"));

	size_t field_index = 0;
	for (size_t flag_index = 0; flag_index < array_size(type_flags); flag_index += 1) {
		for (size_t type_index = 0; type_index < array_size(type_kinds); type_index += 1) {
			str_builder_append(&builder, type_flag_names[flag_index]);
			str_builder_append_char(&builder, ' ');
			str_builder_append(&builder, type_kind_names[type_index]);
			str_builder_append_char(&builder, ' ');
			str_builder_append(&builder, STR_LIT("field"));
			str_builder_append_int(&builder, field_index);
			str_builder_append(&builder, STR_LIT(";\n"));

			field_index += 1;
		}
	}

	str_builder_append(&builder, STR_LIT("};\n"));

	printf("%.*s\n", STR_FMT(builder.string));

	SourceStorage source_storage;
	Diagnostics diagnostics;
	ParsedAST ast;
	run_parser_test(context, &diagnostics, &source_storage, builder.string, &ast);

	ParsedNode* first_def = ast.root_nodes.first;
	assert(first_def->kind == AST_NODE_STRUCT);

	ParsedStruct* struct_def = first_def->struct_def;
	assert(struct_def->field_count == field_index);

	{
		size_t field_index = 0;
		for (size_t flag_index = 0; flag_index < array_size(type_flags); flag_index += 1) {
			for (size_t type_index = 0; type_index < array_size(type_kinds); type_index += 1) {
				const ParsedStructField* field = &struct_def->fields[field_index];
				ParsedTypeKind type_kind = type_kinds[type_index] | type_flags[flag_index];

				assert(field->type.kind == type_kind);

				field_index += 1;
			}
		}
	}
}

void test_parse_variable_declaration(TestContext* context) {
	SourceStorage source_storage;
	Diagnostics diagnostics;
	ParsedAST ast;
	run_parser_test(context, &diagnostics, &source_storage, STR_LIT("int a;"), &ast);

	diagnostics_print(&diagnostics);
	assert(diagnostics.first == NULL);
	assert(ast.root_nodes.count == 1);

	ParsedNode* first_def = ast.root_nodes.first;
	assert(first_def->kind == AST_NODE_VARIABLE);

	ParsedVariable* variable = &first_def->variable;
	assert(str_equal(variable->name.string, STR_LIT("a")));
	assert(variable->value == NULL);
	assert(variable->type.kind == PARSED_TYPE_INT);
}

void test_parse_simple_bin_expr(TestContext* context) {
	SourceStorage source_storage;
	Diagnostics diagnostics;
	ParsedAST ast;
	run_parser_test(context, &diagnostics, &source_storage, STR_LIT("0xff + 10;"), &ast);

	diagnostics_print(&diagnostics);
	assert(diagnostics.first == NULL);
	assert(ast.root_nodes.count == 1);

	assert(ast.root_nodes.first->kind == AST_NODE_EXPR);
	ParsedExpr* expr = &ast.root_nodes.first->expr;
	assert(expr->kind == EXPR_BINARY);
	assert(expr->binary.op == BIN_OP_ADD);

	ParsedExpr* left = expr->binary.left;
	ParsedExpr* right = expr->binary.right;

	assert(left->kind == EXPR_INTEGER_LITERAL);
	assert(left->int_literal.integer_type == PARSED_TYPE_INT);
	assert(left->int_literal.format == INT_LIT_FMT_HEX);
	assert(left->int_literal.value == 255);

	assert(right ->kind == EXPR_INTEGER_LITERAL);
	assert(right->int_literal.integer_type == PARSED_TYPE_INT);
	assert(right->int_literal.format == INT_LIT_FMT_DECIMAL);
	assert(right->int_literal.value == 10);
}

void test_bin_op_precedence(TestContext* context) {
	SourceStorage source_storage;
	Diagnostics diagnostics;
	ParsedAST ast;
	run_parser_test(context, &diagnostics, &source_storage, STR_LIT("0xff + 10 * 99 + 02;"), &ast);

	diagnostics_print(&diagnostics);
	assert(diagnostics.first == NULL);
	assert(ast.root_nodes.count == 1);

	assert(ast.root_nodes.first->kind == AST_NODE_EXPR);

	ParsedExpr* expr = &ast.root_nodes.first->expr;
	assert(expr->kind == EXPR_BINARY);

	assert(expr->binary.left->kind == EXPR_INTEGER_LITERAL);
	assert(expr->binary.left->int_literal.value == 255);

	assert(expr->binary.right->kind == EXPR_BINARY);

	ParsedExpr* inner_expr = expr->binary.right;
	assert(inner_expr->kind == EXPR_BINARY);
	assert(inner_expr->binary.right->kind == EXPR_INTEGER_LITERAL);
	assert(inner_expr->binary.right->int_literal.value == 2);

	ParsedExpr* product_expr = inner_expr->binary.left;
	assert(product_expr->binary.left->kind == EXPR_INTEGER_LITERAL);
	assert(product_expr->binary.right->kind == EXPR_INTEGER_LITERAL);
}

void test_parse_variable_ref_expr(TestContext* context) {
	SourceStorage source_storage;
	Diagnostics diagnostics;
	ParsedAST ast;
	run_parser_test(context, &diagnostics, &source_storage, STR_LIT("int a; a + a;"), &ast);

	diagnostics_print(&diagnostics);
	assert(diagnostics.first == NULL);
	assert(ast.root_nodes.count == 2);

	ParsedNode* first_node = ast.root_nodes.first;
	ParsedNode* second_node = first_node->next;

	assert(first_node->kind == AST_NODE_VARIABLE);
	assert(second_node->kind == AST_NODE_EXPR);

	ParsedVariable* variable = &first_node->variable;

	ParsedExpr* expr = &second_node->expr;
	assert(expr->kind == EXPR_BINARY);

	assert(expr->binary.left->kind == EXPR_VARIABLE_REFERENCE);
	assert(expr->binary.left->variable_ref == variable);
	assert(expr->binary.right->kind == EXPR_VARIABLE_REFERENCE);
	assert(expr->binary.right->variable_ref == variable);
}

void test_parse_return_stmt(TestContext* context) {
	SourceStorage source_storage;
	Diagnostics diagnostics;
	ParsedAST ast;
	run_parser_test(context, &diagnostics, &source_storage, STR_LIT("int main() { return 0; }"), &ast);

	diagnostics_print(&diagnostics);
	assert(ast.root_nodes.count == 1);

	ParsedNode* node = ast.root_nodes.first;
	assert(node->kind == AST_NODE_FUNCTION);

	ParsedFunction* function = node->function_def;
	ParsedScope* body = function->body;
	assert(body->nodes.count == 1);

	ParsedNode* return_node = body->nodes.first;
	assert(return_node->kind == AST_NODE_RETURN);
	assert(return_node->return_stmt.value);
}

void test_parse_return_stmt_without_value(TestContext* context) {
	SourceStorage source_storage;
	Diagnostics diagnostics;
	ParsedAST ast;
	run_parser_test(context, &diagnostics, &source_storage, STR_LIT("void main() { return; }"), &ast);

	diagnostics_print(&diagnostics);
	assert(ast.root_nodes.count == 1);

	ParsedNode* node = ast.root_nodes.first;
	assert(node->kind == AST_NODE_FUNCTION);

	ParsedFunction* function = node->function_def;
	ParsedScope* body = function->body;
	assert(body->nodes.count == 1);

	ParsedNode* return_node = body->nodes.first;
	assert(return_node->kind == AST_NODE_RETURN);
	assert(return_node->return_stmt.value == NULL);
}

void test_multi_part_string_merging(TestContext* context) {
	SourceStorage source_storage;
	Diagnostics diagnostics;
	ParsedAST ast;
	run_parser_test(context, &diagnostics, &source_storage, STR_LIT("const char* s = \"hello\" \"world\";"), &ast);

	diagnostics_print(&diagnostics);
	assert(ast.root_nodes.count == 1);

	ParsedNode* node = ast.root_nodes.first;
	assert(node->kind == AST_NODE_VARIABLE);

	ParsedVariable* var = &node->variable;
	assert(var->value != NULL);
	assert(var->value->kind == EXPR_STRING_LITERAL);
	assert(str_equal(var->value->string_literal.full_string, STR_LIT("helloworld")));
}

void test_parse_expr_inside_parens(TestContext* context) {
	SourceStorage source_storage;
	Diagnostics diagnostics;
	ParsedAST ast;
	run_parser_test(context, &diagnostics, &source_storage, STR_LIT("int a = ((10) + 1);"), &ast);

	diagnostics_print(&diagnostics);
	assert(ast.root_nodes.count == 1);

	ParsedNode* node = ast.root_nodes.first;
	assert(node->kind == AST_NODE_VARIABLE);

	ParsedVariable* var = &node->variable;
	assert(var->value != NULL);
	assert(var->value->kind == EXPR_BINARY);

	ParsedExpr* bin_expr = var->value;
	assert(bin_expr->binary.left->kind == EXPR_INTEGER_LITERAL);
	assert(bin_expr->binary.left->int_literal.value == 10);

	assert(bin_expr->binary.right->kind == EXPR_INTEGER_LITERAL);
	assert(bin_expr->binary.right->int_literal.value == 1);
}

void test_allow_variable_shadowing_in_nested_blocks(TestContext* context) {
	SourceStorage source_storage;
	Diagnostics diagnostics;
	ParsedAST ast;
	run_parser_test(context, &diagnostics, &source_storage, STR_LIT(
				"int a = 1;"
				"{"
				"	int a = 10;"
				"}"
				), &ast);

	diagnostics_print(&diagnostics);
	assert(diagnostics.first == NULL);
	assert(ast.root_nodes.count == 2);

	ParsedNode* node = ast.root_nodes.first;
	assert(node->kind == AST_NODE_VARIABLE);

	ParsedVariable* var = &node->variable;
	assert(var->value != NULL);
	assert(var->value->kind == EXPR_INTEGER_LITERAL);
	assert(var->value->int_literal.value == 1);

	ParsedNode* block_node = node->next;
	assert(block_node->block.nodes.count == 1);

	ParsedNode* var2_node = block_node->block.nodes.first;
	assert(var2_node->kind == AST_NODE_VARIABLE);

	ParsedVariable* var2 = &var2_node->variable;
	assert(var2->value != NULL);
	assert(var2->value->kind == EXPR_INTEGER_LITERAL);
	assert(var2->value->int_literal.value == 10);
}

void test_allow_variable_shadowing_in_if_statements(TestContext* context) {
	SourceStorage source_storage;
	Diagnostics diagnostics;
	ParsedAST ast;
	run_parser_test(context, &diagnostics, &source_storage, STR_LIT(
				"int a = 1;"
				"if (1)"
				"	int a = 10;"
				), &ast);

	diagnostics_print(&diagnostics);
	assert(diagnostics.first == NULL);
	assert(ast.root_nodes.count == 2);

	ParsedNode* node = ast.root_nodes.first;
	assert(node->kind == AST_NODE_VARIABLE);

	ParsedVariable* var = &node->variable;
	assert(var->value != NULL);
	assert(var->value->kind == EXPR_INTEGER_LITERAL);
	assert(var->value->int_literal.value == 1);

	ParsedNode* if_stmt_node = node->next;
	assert(if_stmt_node->kind == AST_NODE_IF);
	assert(if_stmt_node->if_stmt.true_node != NULL);

	ParsedNode* var2_node = if_stmt_node->if_stmt.true_node;
	assert(var2_node->kind == AST_NODE_VARIABLE);

	ParsedVariable* var2 = &var2_node->variable;
	assert(var2->value != NULL);
	assert(var2->value->kind == EXPR_INTEGER_LITERAL);
	assert(var2->value->int_literal.value == 10);
}

void test_allow_variable_shadowing_in_else_branch_if_statements(TestContext* context) {
	SourceStorage source_storage;
	Diagnostics diagnostics;
	ParsedAST ast;
	run_parser_test(context, &diagnostics, &source_storage, STR_LIT(
				"int a = 1;"
				"if (1) {}"
				"else"
				"	int a = 10;"
				), &ast);

	diagnostics_print(&diagnostics);
	assert(diagnostics.first == NULL);
	assert(ast.root_nodes.count == 2);

	ParsedNode* node = ast.root_nodes.first;
	assert(node->kind == AST_NODE_VARIABLE);

	ParsedVariable* var = &node->variable;
	assert(var->value != NULL);
	assert(var->value->kind == EXPR_INTEGER_LITERAL);
	assert(var->value->int_literal.value == 1);

	ParsedNode* if_stmt_node = node->next;
	assert(if_stmt_node->kind == AST_NODE_IF);
	assert(if_stmt_node->if_stmt.true_node != NULL);

	ParsedNode* var2_node = if_stmt_node->if_stmt.false_node;
	assert(var2_node->kind == AST_NODE_VARIABLE);

	ParsedVariable* var2 = &var2_node->variable;
	assert(var2->value != NULL);
	assert(var2->value->kind == EXPR_INTEGER_LITERAL);
	assert(var2->value->int_literal.value == 10);
}

void test_parse_recursive_function(TestContext* context) {
	SourceStorage source_storage;
	Diagnostics diagnostics;
	ParsedAST ast;
	run_parser_test(context,
			&diagnostics,
			&source_storage,
			STR_LIT("void main() { main(); }"),
			&ast);

	diagnostics_print(&diagnostics);
	assert(diagnostics.first == NULL);
	assert(ast.root_nodes.count == 1);

	ParsedNode* node = ast.root_nodes.first;
	assert(node->kind == AST_NODE_FUNCTION);

	assert(node->function_def->body);
	const ParsedFunction* func = node->function_def;
	
	assert(func->body->nodes.count == 1);
	const ParsedNode* call = func->body->nodes.first;
	assert(call->kind == AST_NODE_EXPR);
	assert(call->expr.kind == EXPR_CALL);

	const ParsedExpr* callable = call->expr.call.callable;
	assert(callable->kind == EXPR_FUNCTION_REFERENCE);
	assert(callable->function_ref == func);
}

void test_parse_function_param_in_expr(TestContext* context) {
	SourceStorage source_storage;
	Diagnostics diagnostics;
	ParsedAST ast;
	run_parser_test(context,
			&diagnostics,
			&source_storage,
			STR_LIT("void main(int argc) { argc; }"),
			&ast);

	diagnostics_print(&diagnostics);
	assert(diagnostics.first == NULL);
	assert(ast.root_nodes.count == 1);

	ParsedNode* node = ast.root_nodes.first;
	assert(node->kind == AST_NODE_FUNCTION);

	assert(node->function_def->body);
	const ParsedFunction* func = node->function_def;
	
	assert(func->body->nodes.count == 1);
	const ParsedNode* expr = func->body->nodes.first;
	assert(expr->kind == AST_NODE_EXPR);
	assert(expr->expr.kind == EXPR_FUNCTION_PARAM);

	assert(expr->expr.function_param.function_def == func);
	assert(expr->expr.function_param.param_index == 0);
}

void test_register_unnamed_function_param(TestContext* context) {
	SourceStorage source_storage;
	Diagnostics diagnostics;
	ParsedAST ast;
	run_parser_test(context,
			&diagnostics,
			&source_storage,
			STR_LIT("void main(int) {}"),
			&ast);

	diagnostics_print(&diagnostics);
	assert(diagnostics.first == NULL);
	assert(ast.root_nodes.count == 1);
}

static void _run_anonymous_type_declaration_sub_test(TestContext* context, String source_code) {
	SourceStorage source_storage;

	source_storage_init(&source_storage, (StringArray) {}, context->arena);
	SourceFile* source_file = source_storage_append(&source_storage, STR_LIT(DEFAULT_SOURCE_PATH), source_code);

	Diagnostics diagnostics = (Diagnostics) {
		.allocator = context->temp_arena,
	};

	Arena generated_tokens_arena = arena_alloc_sub_arena(context->arena, 2 * 4096);

	Preprocessor preprocessor = {};
	preprocessor_init(&preprocessor,
			&source_storage,
			source_file,
			&diagnostics,
			context->arena,
			context->temp_arena,
			&generated_tokens_arena);

	IdentifierStorage ident_storage;

	// NOTE: This arena is used by the `IdentifierStorage` inside the parser,
	//       so it's lifetime is no longer than the one of the parser.
	Arena ident_arena = arena_alloc_sub_arena(context->arena, 16 * 1024);
	Arena ast_arena = arena_alloc_sub_arena(context->arena, 16 * 1024);

	ident_storage_init(&ident_storage, arena_allocator_new(&ident_arena), &ident_arena);

	Parser parser = {};
	parser_init(&parser, &ast_arena, context->temp_arena, &ident_storage, &preprocessor, &diagnostics);

	ParsedAST ast;
	parser_parse(&parser, &ast);

	// WARN: Work's under the assuption that the root scope of `ident_storage`
	//       isn't cleared after finishing the parshing.
	IdentifierEntry* entry = ident_storage_find(&ident_storage,
			IDENT_NAMESPACE_TAGGED,
			IDENT_FIND_IN_ALL_PARENT_SCOPES,
			STR_LIT("Anonymous"));

	assert(entry == NULL);
}

void test_inner_struct_decl_is_anonymous(TestContext* context) {
	_run_anonymous_type_declaration_sub_test(context,
			STR_LIT("struct Hello { struct Anonymous {} inner; };"));
}

void test_inner_enum_decl_is_anonymous(TestContext* context) {
	_run_anonymous_type_declaration_sub_test(context,
			STR_LIT("struct Outer { enum Anonymous {} inner; };"));
}

void test_map_current_struct_fields(TestContext* context) {
	String source_code = STR_LIT("struct Struct {\n"
			"	int value0;\n"
			"	int value1;\n"
			"	float value2;\n"
			"	float;\n"
			"};");

	SourceStorage source_storage;
	Diagnostics diagnostics;
	ParsedAST ast;
	IdentifierStorage* ident_storage;
	run_parser_test_2(context,
			&diagnostics,
			&source_storage,
			source_code,
			&ast,
			&ident_storage);

	diagnostics_print(&diagnostics);
	assert(diagnostics.first == NULL);
	assert(ast.root_nodes.count == 1);

	IdentifierEntry* entry = ident_storage_find(ident_storage,
			IDENT_NAMESPACE_TAGGED,
			IDENT_FIND_IN_ALL_PARENT_SCOPES,
			STR_LIT("Struct"));

	assert(entry != NULL);

	const ParsedStruct* struct_def = entry->struct_def;
	assert(entry != NULL);

	const ParsedStructFieldNamespace* fields = struct_def->field_namespace;
	assert(fields != NULL);
	assert(fields->size == 3);

	String expected_fields[] = {
		STR_LIT("value0"),
		STR_LIT("value1"),
		STR_LIT("value2"),
	};

	for (size_t i = 0; i < array_size(expected_fields); i += 1) {
		size_t index = struct_field_namespace_index_of(fields, expected_fields[i]);
		assert(index != SIZE_MAX);

		assert(fields->entries[index].struct_def == struct_def);
		assert(fields->entries[index].field_index == i);
	}
}

void test_map_current_and_inner_anonymous_struct_fields(TestContext* context) {
	String source_code = STR_LIT("struct Struct {\n"
			"	int value0;\n"
			"	struct Inner {\n"
			"		int inner_value0;\n"
			"		struct InnerMost {\n"
			"			float inner_most_value0;\n"
			"		};\n"
			"	};\n"
			"	int value1;\n"
			"	float value2;\n"
			"	float;\n"
			"};");

	SourceStorage source_storage;
	Diagnostics diagnostics;
	ParsedAST ast;
	IdentifierStorage* ident_storage;
	run_parser_test_2(context,
			&diagnostics,
			&source_storage,
			source_code,
			&ast,
			&ident_storage);

	diagnostics_print(&diagnostics);
	assert(diagnostics.first == NULL);
	assert(ast.root_nodes.count == 1);

	IdentifierEntry* entry = ident_storage_find(ident_storage,
			IDENT_NAMESPACE_TAGGED,
			IDENT_FIND_IN_ALL_PARENT_SCOPES,
			STR_LIT("Struct"));

	assert(entry != NULL);

	const ParsedStruct* struct_def = entry->struct_def;
	assert(entry != NULL);

	const ParsedStructFieldNamespace* fields = struct_def->field_namespace;
	assert(fields != NULL);
	assert(fields->size == 5);

	String expected_fields[] = {
		STR_LIT("value0"),
		STR_LIT("inner_value0"),
		STR_LIT("inner_most_value0"),
		STR_LIT("value1"),
		STR_LIT("value2"),
	};

	for (size_t i = 0; i < array_size(expected_fields); i += 1) {
		size_t index = struct_field_namespace_index_of(fields, expected_fields[i]);
		assert(index != SIZE_MAX);
	}
}

void test_fields_not_mapped_for_struct_defined_inline_with_the_named_field(TestContext* context) {
	String source_code = STR_LIT("struct Struct {\n"
			"	int value0;\n"
			"	struct Inner {\n"
			"		int inner_value0;\n"
			"		struct InnerMost {\n"
			"			float inner_most_value0;\n"
			"		} inner;\n"
			"	};\n"
			"	int value1;\n"
			"	float value2;\n"
			"	float;\n"
			"};");

	SourceStorage source_storage;
	Diagnostics diagnostics;
	ParsedAST ast;
	IdentifierStorage* ident_storage;
	run_parser_test_2(context,
			&diagnostics,
			&source_storage,
			source_code,
			&ast,
			&ident_storage);

	diagnostics_print(&diagnostics);
	assert(diagnostics.first == NULL);
	assert(ast.root_nodes.count == 1);

	IdentifierEntry* entry = ident_storage_find(ident_storage,
			IDENT_NAMESPACE_TAGGED,
			IDENT_FIND_IN_ALL_PARENT_SCOPES,
			STR_LIT("Struct"));

	assert(entry != NULL);

	const ParsedStruct* struct_def = entry->struct_def;
	assert(entry != NULL);

	const ParsedStructFieldNamespace* fields = struct_def->field_namespace;
	assert(fields != NULL);
	assert(fields->size == 5);

	String expected_fields[] = {
		STR_LIT("value0"),
		STR_LIT("inner_value0"),
		STR_LIT("inner"),
		STR_LIT("value1"),
		STR_LIT("value2"),
	};

	for (size_t i = 0; i < array_size(expected_fields); i += 1) {
		size_t index = struct_field_namespace_index_of(fields, expected_fields[i]);
		assert(index != SIZE_MAX);
	}

	String unexpected_fields[] = {
		STR_LIT("inner_most_value0"),
	};

	for (size_t i = 0; i < array_size(unexpected_fields); i += 1) {
		size_t index = struct_field_namespace_index_of(fields, unexpected_fields[i]);
		assert(index == SIZE_MAX);
	}
}

void test_parse_union_def(TestContext* context) {
	String source_code = STR_LIT("union Union { int i; float f; };");

	SourceStorage source_storage;
	Diagnostics diagnostics;
	ParsedAST ast;
	IdentifierStorage* ident_storage;
	run_parser_test_2(context,
			&diagnostics,
			&source_storage,
			source_code,
			&ast,
			&ident_storage);

	diagnostics_print(&diagnostics);
	assert(diagnostics.first == NULL);
	assert(ast.root_nodes.count == 1);

	IdentifierEntry* entry = ident_storage_find(ident_storage,
			IDENT_NAMESPACE_TAGGED,
			IDENT_FIND_IN_ALL_PARENT_SCOPES,
			STR_LIT("Union"));

	assert(entry != NULL);

	assert(entry->kind == IDENT_UNION);

	const ParsedStruct* union_def = entry->union_def;
	assert(union_def->layout_kind == STRUCT_LAYOUT_KIND_UNION);
}

inline Token _create_string_token(String str, const SourceFile* file) {
	return (Token) { .kind = TOKEN_STRING,
		.string = str,
		.source_range = source_string_to_range((SourceString) {
				.string = str,
				.source_file = file
				})
	};
}

void test_parse_int_literal_sufixes(TestContext* context) {
	IntegerLiteralSufixKind std_sufix_kinds[] = {
		INT_SUFIX_U,
		INT_SUFIX_L,
		INT_SUFIX_UL,
		INT_SUFIX_LL,
		INT_SUFIX_ULL,
	};

	String std_sufixes[] = {
		STR_LIT("u"),
		STR_LIT("l"),
		STR_LIT("ul"),
		STR_LIT("ll"),
		STR_LIT("ull"),
	};

	String int_lit = STR_LIT("100");
	uint64_t int_lit_value = 100;
	for (size_t i = 0; i < array_size(std_sufix_kinds); i += 1) {
		ArenaRegion temp = arena_begin_temp(context->temp_arena);
		Diagnostics diagnostics = { .allocator = context->temp_arena };

		StringBuilder builder = { .arena = context->temp_arena };
		str_builder_append(&builder, int_lit);
		str_builder_append(&builder, std_sufixes[i]);

		SourceFile source_file = (SourceFile) {
			.path = STR_LIT(DEFAULT_SOURCE_PATH),
			.source_code = builder.string,
			.line_info = line_info_from_source(context->temp_arena, builder.string),
		};

		IntLiteral literal = {};
		bool parsed = parse_int_literal(
				_create_string_token(builder.string, &source_file), 
				&diagnostics,
				&literal);

		assert(parsed);

		assert(literal.has_sufix);
		assert(literal.sufix_bit_count == 0);
		assert(literal.sufix_kind == std_sufix_kinds[i]);
		assert(literal.value == int_lit_value);
		assert(diagnostics.first == NULL);

		arena_end_temp(temp);
	}
}

void test_parse_int_literal_sufixes_with_bit_count(TestContext* context) {
	size_t bit_count[] = { 8, 16, 32, 64 };
	IntegerLiteralSufixKind std_sufix_kinds[] = {
		INT_SUFIX_NONE,
		INT_SUFIX_U,
	};

	String std_sufixes[] = {
		STR_LIT(""),
		STR_LIT("u"),
	};

	String int_lit = STR_LIT("100");

	for (size_t i = 0; i < array_size(std_sufix_kinds); i += 1) {
		for (size_t j = 0; j < array_size(bit_count); j += 1) {
			ArenaRegion temp = arena_begin_temp(context->temp_arena);
			Diagnostics diagnostics = { .allocator = context->temp_arena };

			StringBuilder builder = { .arena = context->temp_arena };
			str_builder_append(&builder, int_lit);
			str_builder_append(&builder, std_sufixes[i]);
			str_builder_append_char(&builder, 'i');
			str_builder_append_int(&builder, bit_count[j]);

			SourceFile source_file = (SourceFile) {
				.path = STR_LIT(DEFAULT_SOURCE_PATH),
				.source_code = builder.string,
				.line_info = line_info_from_source(context->temp_arena, builder.string),
			};

			IntLiteral literal = {};
			bool parsed = parse_int_literal(
					_create_string_token(builder.string, &source_file), 
					&diagnostics,
					&literal);

			assert(parsed);

			assert(literal.has_sufix);
			assert(literal.sufix_bit_count == bit_count[j]);
			assert(literal.sufix_kind == std_sufix_kinds[i]);
			assert(diagnostics.first == NULL);

			arena_end_temp(temp);
		}
	}
}

void test_invalid_int_literal_sufixes(TestContext* context) {
	String std_sufixes[] = {
		STR_LIT("l"),
		STR_LIT("ul"),
		STR_LIT("ll"),
		STR_LIT("ull"),
	};

	size_t bit_count[] = { 8, 16, 32, 64 };

	String int_lit = STR_LIT("100");
	for (size_t i = 0; i < array_size(std_sufixes); i += 1) {
		for (size_t j = 0; j < array_size(bit_count); j += 1) {
			ArenaRegion temp = arena_begin_temp(context->temp_arena);

			Diagnostics diagnostics = { .allocator = context->temp_arena };
			StringBuilder builder = { .arena = context->temp_arena };
			str_builder_append(&builder, int_lit);
			str_builder_append(&builder, std_sufixes[i]);
			str_builder_append_char(&builder, 'i');
			str_builder_append_int(&builder, bit_count[j]);

			SourceFile source_file = (SourceFile) {
				.path = STR_LIT(DEFAULT_SOURCE_PATH),
				.source_code = builder.string,
				.line_info = line_info_from_source(context->temp_arena, builder.string),
			};

			IntLiteral literal = {};
			bool parsed = parse_int_literal(
					_create_string_token(builder.string, &source_file), 
					&diagnostics,
					&literal);

			assert(!parsed);

			assert(!literal.has_sufix);
			assert(literal.sufix_bit_count == 0);
			assert(literal.sufix_kind == INT_SUFIX_NONE);
			assert(diagnostics.first != NULL);

			SourceRange sufix_range = diagnostics.first->highlighted_ranges[0];
			assert(sufix_range.start == int_lit.length);
			assert(sufix_range.end == builder.string.length);

			arena_end_temp(temp);
		}
	}
}

void test_parse_type_of_int_literal_with_sufix(TestContext* context) {
	String sub_tests[] = {
		STR_LIT("1i8;"),
		STR_LIT("1i16;"),
		STR_LIT("1i32;"),
		STR_LIT("1i64;"),

		STR_LIT("1ui8;"),
		STR_LIT("1ui16;"),
		STR_LIT("1ui32;"),
		STR_LIT("1ui64;"),

		STR_LIT("1u;"),
		STR_LIT("1l;"),
		STR_LIT("1ul;"),
		STR_LIT("1ll;"),
		STR_LIT("1ull;"),
	};

	ParsedTypeKind expected_types[] = {
		PARSED_TYPE_INT8,
		PARSED_TYPE_INT16,
		PARSED_TYPE_INT32,
		PARSED_TYPE_INT64,

		PARSED_TYPE_UNSIGNED_INT8,
		PARSED_TYPE_UNSIGNED_INT16,
		PARSED_TYPE_UNSIGNED_INT32,
		PARSED_TYPE_UNSIGNED_INT64,

		PARSED_TYPE_UNSIGNED_INT,
		PARSED_TYPE_LONG,
		PARSED_TYPE_UNSIGNED_LONG,
		PARSED_TYPE_LONG_LONG,
		PARSED_TYPE_UNSIGNED_LONG_LONG,
	};

	static_assert(array_size(sub_tests) == array_size(expected_types),
			"Number of expected results doesn't match the number of sub tests");

	for (size_t i = 0; i < array_size(sub_tests); i += 1) {
		ArenaRegion temp1 = arena_begin_temp(context->arena);
		ArenaRegion temp2 = arena_begin_temp(context->temp_arena);

		Diagnostics diagnostics;
		SourceStorage source_storage;
		ParsedAST ast;

		run_parser_test(context, &diagnostics, &source_storage, sub_tests[i], &ast);
		assert(diagnostics.first == NULL);

		assert(ast.root_nodes.count == 1);
		ParsedNode* first = ast.root_nodes.first;
		assert(first->kind == AST_NODE_EXPR);

		ParsedExpr* expr = &first->expr;
		assert(expr->kind == EXPR_INTEGER_LITERAL);
		assert(expr->int_literal.format == INT_LIT_FMT_DECIMAL);
		assert(expr->int_literal.integer_type == expected_types[i]);
		assert(expr->int_literal.value == 1);

		arena_end_temp(temp2);
		arena_end_temp(temp1);
	}
}
