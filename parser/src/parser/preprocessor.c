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

bool _preprocessor_parse_macro(Preprocessor* state, MacroDefinition* macro) {
	ArenaRegion temp_region = arena_begin_temp(state->allocator);

	Token name_token = tokenizer_next_token(&state->tokenizer);
	if (name_token.kind != TOKEN_IDENT) {
		diagnostics_report_error(state->diagnostics, name_token.source_range, STR_LIT("Expected macro name"));
		return false;
	}

	macro->name = name_token.string;

	Token maybe_paren = tokenizer_view_next(&state->tokenizer);
	if (maybe_paren.kind == TOKEN_LEFT_PAREN) {
		// We have a function style macro => parse paremeter names
		tokenizer_reset_to_token(&state->tokenizer, maybe_paren);

		macro->parameter_names = arena_alloc_array(state->allocator, String, 0);

		while (true) {
			Token token = tokenizer_next_token(&state->tokenizer);
			if (token.kind == TOKEN_IDENT) {
				String* param_name = arena_alloc(state->allocator, String);			
				*param_name = token.string;
				macro->parameter_count += 1;

				token = tokenizer_next_token(&state->tokenizer);
				if (token.kind == TOKEN_COMMA) {
					// There are potentially more paremeters
					continue;
				}
			}

			if (token.kind == TOKEN_RIGHT_PAREN) {
				break;
			} else {
				TokenKind expected_tokens[] = {
					TOKEN_COMMA,
					TOKEN_RIGHT_PAREN,
				};

				diagnostics_report_unexpected_token(state->diagnostics, token, expected_tokens, array_size(expected_tokens));
				arena_end_temp(temp_region);
				return false;
			}
		}
	}

	return true;
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
	} else if (str_equal(next_token.string, STR_LIT("define"))) {
		MacroDefinition macro = {};
		if (_preprocessor_parse_macro(state, &macro)) {

		}
	} else {
		StringBuilder builder = { .arena = state->diagnostics->allocator };
		str_builder_append(&builder, STR_LIT("Unhandled preprocessor derective type: "));
		str_builder_append(&builder, next_token.string);

		diagnostics_report_error(state->diagnostics,
				next_token.source_range,
				builder.string);
	}
}

