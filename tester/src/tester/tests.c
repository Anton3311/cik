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
			source_code,
			out_line_info,
			out_diagnostics,
			context->arena,
			context->temp_arena);
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
