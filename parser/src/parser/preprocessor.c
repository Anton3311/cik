#include "preprocessor.h"

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
		unreachable_msg("Unhandled preprocessor derective type");
	}
}

