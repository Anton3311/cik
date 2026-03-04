#include "tests.h"

#include "parser/preprocessor.h"

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

void test_token_source_range_matches_token_string(TestContext* context) {
	assert(TOKEN_EOF == 0);

	size_t token_count = 4096;
	size_t acceptable_token_count = TOKEN_COUNT - 1; // - 1 because TOKEN_EOF is exluded

	StringBuilder builder = { .arena = context->temp_arena };
	for (size_t i = 0; i < token_count; i += 1) {
		size_t token_index = (size_t)rand() % acceptable_token_count;
		
		TokenKind token_kind = (TokenKind)(1 + token_index);
		assert(token_kind != TOKEN_EOF);

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

	Tokenizer tokenizer = {
		.source_code = builder.string,
	};

	size_t generated_token_count = 0;
	while (true) {
		Token token = tokenizer_next_token(&tokenizer);
		if (token.kind == TOKEN_EOF) {
			break;
		}

		generated_token_count += 1;

		String source_sub_str = sub_str(builder.string,
				token.source_range.start,
				token.source_range.end - token.source_range.start);

		assert(str_equal(source_sub_str, token.string));
	}

	assert(generated_token_count == token_count);
}

//
// Preprocessor
//

#define DEFAULT_SOURCE_PATH "test.c"

static void init_preprocessor_test(TestContext* context,
		String source_code,
		Preprocessor* out_preprocessor,
		Diagnostics* out_diagnostics,
		LineInfo* out_line_info) {

	*out_line_info = line_info_from_source(context->arena, source_code);

	*out_diagnostics = (Diagnostics) {
		.allocator = context->arena,
		.source_code = source_code,
		.line_info = *out_line_info,
	};
	
	preprocessor_init(out_preprocessor,
			STR_LIT(DEFAULT_SOURCE_PATH),
			source_code,
			out_line_info,
			out_diagnostics,
			context->arena,
			context->temp_arena,
			context->arena);
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
