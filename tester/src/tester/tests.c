#include "tests.h"

#include "parser/preprocessor.h"

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
