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

void test_non_function_style_macro_expansion(TestContext* context) {
	String source_code = STR_LIT("#define hello 100 + 100\nhello");
	String expected_source_code = STR_LIT("100 + 100");

	LineInfo line_info = line_info_from_source(context->arena, source_code);

	Diagnostics diagnostics = (Diagnostics) {
		.allocator = context->arena,
		.source_code = source_code,
		.line_info = line_info,
	};
	
	Preprocessor preprocessor = {};
	preprocessor_init(&preprocessor, source_code, &line_info, &diagnostics, context->arena);

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

void test_fail(TestContext* context) {
	assert(false);
}

void test_success(TestContext* context) {
	assert(true);
}

