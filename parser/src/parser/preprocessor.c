#include "preprocessor.h"

//
// Preprocessor
//

void preprocessor_init(Preprocessor* state, String source_code, Arena* allocator) {
	state->line_info = line_info_from_source(allocator, source_code);
	state->tokenizer = (Tokenizer) {
		.source_code = source_code,
		.read_position = 0,
	};
}

void preprocessor_skip_derective(Preprocessor* state) {
	Token next_token = tokenizer_next_token(&state->tokenizer);
	assert(next_token.kind == TOKEN_IDENT);

	if (str_equal(next_token.string, STR_LIT("include"))) {
		tokenizer_skip_whitespace_and_comments(&state->tokenizer);

		if (_tokenizer_has_next_char(&state->tokenizer, '<')) {
			Token string_token = {};
			StringTokenizerResult result = _tokenizer_try_create_string_token(&state->tokenizer, '<', '>', &string_token);
			assert(result == STR_TOKEN_RESULT_NONE);
		} else if (_tokenizer_has_next_char(&state->tokenizer, '"')) {
			Token string_token = {};
			StringTokenizerResult result = _tokenizer_try_create_string_token(&state->tokenizer, '"', '"', &string_token);
			assert(result == STR_TOKEN_RESULT_NONE);
		}

		// TODO: Use parsed include statements
	} else {
		StringBuilder builder = { .arena = state->diagnostics->allocator };
		str_builder_append(&builder, STR_LIT("Unhandled preprocessor derective type: "));
		str_builder_append(&builder, next_token.string);

		diagnostics_report_error(state->diagnostics,
				next_token.source_range,
				builder.string);
	}
}

