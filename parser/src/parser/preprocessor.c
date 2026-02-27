#include "preprocessor.h"

void macro_table_append(MacroTable* table, const MacroDefinition* macro) {
	assert(table->count < table->capacity);

	table->macros[table->count] = *macro;
	table->count += 1;
}

const MacroDefinition* macro_table_find(const MacroTable* table, String name) {
	for (size_t i = 0; i < table->count; i += 1) {
		if (str_equal(table->macros[i].name, name)) {
			return &table->macros[i];
		}
	}

	return NULL;
}

//
// Preprocessor
//

void preprocessor_init(Preprocessor* state, String source_code, Arena* allocator) {
	state->line_info = line_info_from_source(allocator, source_code);
	state->tokenizer = (Tokenizer) {
		.source_code = source_code,
		.read_position = 0,
	};

	state->macro_table = (MacroTable) {
		.capacity = 128,
		.count = 0,
	};

	state->macro_table.macros = arena_alloc_array(allocator, MacroDefinition, state->macro_table.capacity);

	state->macro_call_stack_capacity = 32;
	state->macro_call_stack_depth = 0;
	state->macro_call_stack = arena_alloc_array(allocator, MacroCallState, state->macro_call_stack_capacity);
}

void _preprocessor_parse_macro_token_stream(Preprocessor* state, MacroDefinition* macro) {
	macro->tokens = arena_alloc_array(state->allocator, Token, 0);

	SourceRange macro_name_range = source_range_from_sub_string(state->tokenizer.source_code, macro->name);
	uint32_t macro_definition_line = line_info_pos_to_source_location(&state->line_info, macro_name_range.start).line;

	while (true) {
		if (tokenizer_is_end(&state->tokenizer)) {
			break;
		}

		Token token = tokenizer_view_next(&state->tokenizer);
		uint32_t token_line = line_info_pos_to_source_location(&state->line_info, token.source_range.end).line;
		if (token_line == macro_definition_line) {
			// Token is one the same line as the macro definition,
			// so it belongs to the token stream of this macro
			*arena_alloc(state->allocator, Token) = token;
			macro->token_count += 1;

			tokenizer_reset_to_token(&state->tokenizer, token);
		} else {
			break;
		}
	}
}

bool _preprocessor_parse_macro(Preprocessor* state, MacroDefinition* macro) {
	ArenaRegion temp_region = arena_begin_temp(state->allocator);

	Token name_token = tokenizer_next_token(&state->tokenizer);
	if (name_token.kind != TOKEN_IDENT) {
		diagnostics_report_error(state->diagnostics, name_token.source_range, STR_LIT("Expected macro name"));
		return false;
	}

	macro->style = MACRO_STYLE_DEFAULT;
	macro->name = name_token.string;

	Token maybe_paren = tokenizer_view_next(&state->tokenizer);
	if (maybe_paren.kind == TOKEN_LEFT_PAREN) {
		macro->style = MACRO_STYLE_FUNCTION;
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

	_preprocessor_parse_macro_token_stream(state, macro);
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
			macro_table_append(&state->macro_table, &macro);
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

