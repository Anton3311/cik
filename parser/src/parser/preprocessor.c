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

void preprocessor_init(Preprocessor* state,
		String source_code,
		const LineInfo* line_info,
		Diagnostics* diagnostics,
		Arena* allocator,
		Arena* temp_allocator) {
	state->line_info = *line_info;
	state->diagnostics = diagnostics;
	state->allocator = allocator;
	state->temp_allocator = temp_allocator;
	state->tokenizer = (Tokenizer) {
		.source_code = source_code,
		.read_position = 0,
	};

	state->macro_table = (MacroTable) {
		.capacity = 128,
		.count = 0,
	};

	state->macro_table.macros = arena_alloc_array(state->allocator, MacroDefinition, state->macro_table.capacity);

	state->macro_call_stack_capacity = 32;
	state->macro_call_stack_depth = 0;
	state->macro_call_stack = arena_alloc_array(state->allocator, MacroCallState, state->macro_call_stack_capacity);
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

	if (macro->style == MACRO_STYLE_FUNCTION) {
		macro->token_hints = arena_alloc_array(state->allocator, MacroTokenHint, macro->token_count);
		memset(macro->token_hints, 0, sizeof(*macro->token_hints));

		for (size_t i = 0; i < macro->token_count; i += 1) {
			Token token = macro->tokens[i];
			if (token.kind == TOKEN_IDENT) {
				size_t parameter_index = macro_find_param_by_name(macro, token.string);
				if (parameter_index == SIZE_MAX) {
					macro->token_hints[i].kind = MACRO_TOKEN_HINT_NONE;
				} else {
					macro->token_hints[i].parameter_index = parameter_index;
					macro->token_hints[i].kind = MACRO_TOKEN_HINT_PARAMETER;
				}
			} else {
				macro->token_hints[i].kind = MACRO_TOKEN_HINT_NONE;
			}
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

size_t macro_find_param_by_name(const MacroDefinition* macro, String param_name) {
	assert(macro->style == MACRO_STYLE_FUNCTION);

	for (size_t i = 0; i < macro->parameter_count; i += 1) {
		if (str_equal(macro->parameter_names[i], param_name)) {
			return i;
		}
	}

	return SIZE_MAX;
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

bool _preprocessor_try_expand_macro_argument(Preprocessor* state, Token* out_token) {
	assert(state->macro_call_stack_depth > 0);

	MacroCallState* last_call = &state->macro_call_stack[state->macro_call_stack_depth - 1];
	if (last_call->argument_index == SIZE_MAX) {
		return false;
	}

	assert(last_call->argument_index < last_call->macro->parameter_count);

	MacroArgumentTokens argument_tokens = last_call->argument_tokens[last_call->argument_index];
	assert(last_call->argument_token_index <= argument_tokens.count);

	bool argument_is_fully_expanded = last_call->argument_token_index == argument_tokens.count;
	if (argument_is_fully_expanded) {
		last_call->argument_index = SIZE_MAX;
		last_call->argument_token_index = SIZE_MAX;

		// Move to the next token
		last_call->token_index += 1;
		return false;
	}

	*out_token = argument_tokens.tokens[last_call->argument_token_index];
	last_call->argument_token_index += 1;
	return true;
}

bool preprocessor_get_next_macro_expantion_token(Preprocessor* state, Token* out_token) {
	assert(state->macro_call_stack_depth > 0);

	if (_preprocessor_try_expand_macro_argument(state, out_token)) {
		return true;
	}

	MacroCallState* macro_call = NULL;

	// Pop finished macro calls off of the stack
	while (state->macro_call_stack_depth > 0) {
		macro_call = &state->macro_call_stack[state->macro_call_stack_depth - 1];
		if (macro_call->token_index == macro_call->macro->token_count) {
			state->macro_call_stack_depth -= 1;
			macro_call = NULL;
		} else {
			break;
		}
	}

	if (macro_call) {
		Token next_token = macro_call->macro->tokens[macro_call->token_index];

		const MacroDefinition* macro = macro_call->macro;
		MacroTokenHint hint = {};
		if (macro->style == MACRO_STYLE_FUNCTION) {
			assert(macro->token_hints);
			hint = macro->token_hints[macro_call->token_index];
		}

		switch (hint.kind) {
		case MACRO_TOKEN_HINT_NONE:
			*out_token = next_token;
			macro_call->token_index += 1;
			return true;
		case MACRO_TOKEN_HINT_PARAMETER: {
			macro_call->argument_index = hint.parameter_index;
			macro_call->argument_token_index = 0;

			bool has_first_token = _preprocessor_try_expand_macro_argument(state, out_token);
			assert(has_first_token);
			return true;
		}
		}
	}

	return false;
}

MacroCallState* preprocessor_init_macro_call(Preprocessor* state, const MacroDefinition* macro) {
	MacroArgumentTokens* argument_tokens = NULL;

	if (macro->style == MACRO_STYLE_FUNCTION) {
		// Parse macro arguments
		Token maybe_left_paren = tokenizer_next_token(&state->tokenizer);
		if (maybe_left_paren.kind != TOKEN_LEFT_PAREN) {
			TokenKind expected_tokens[] = { TOKEN_LEFT_PAREN };
			diagnostics_report_unexpected_token(state->diagnostics,
					maybe_left_paren,
					expected_tokens,
					array_size(expected_tokens));
			return NULL;
		}

		argument_tokens = arena_alloc_array(state->temp_allocator,
				MacroArgumentTokens,
				macro->parameter_count);

		memset(argument_tokens, 0, sizeof(*argument_tokens) * macro->parameter_count);

		bool end_argument_list = false;

		for (size_t arg_index = 0; arg_index < macro->parameter_count; arg_index += 1) {
			MacroArgumentTokens* tokens = &argument_tokens[arg_index];
			tokens->tokens = arena_alloc_array(state->temp_allocator, Token, 0);
			
			while (true) {
				Token token = tokenizer_view_next(&state->tokenizer);
				if (token.kind == TOKEN_COMMA) {
					tokenizer_reset_to_token(&state->tokenizer, token);
					break;
				} else if (token.kind == TOKEN_RIGHT_PAREN) {
					end_argument_list = true;
					break;
				}

				// consume the current token and put it in list of tokens
				// of the current argument
				tokenizer_reset_to_token(&state->tokenizer, token);

				arena_alloc(state->temp_allocator, Token);

				tokens->tokens[tokens->count] = token;
				tokens->count += 1;
			}

			if (end_argument_list) {
				assert(arg_index == macro->parameter_count - 1);
				break;
			}
		}

		Token maybe_right_paren = tokenizer_next_token(&state->tokenizer);
		if (maybe_right_paren.kind != TOKEN_RIGHT_PAREN) {
			TokenKind expected_tokens[] = { TOKEN_RIGHT_PAREN };
			diagnostics_report_unexpected_token(state->diagnostics,
					maybe_right_paren,
					expected_tokens,
					array_size(expected_tokens));
			return NULL;
		}
	}

	if (macro->token_count == 0) {
		return NULL;
	}

	assert(state->macro_call_stack_depth < state->macro_call_stack_capacity);

	MacroCallState* macro_call = &state->macro_call_stack[state->macro_call_stack_depth];
	state->macro_call_stack_depth += 1;

	memset(macro_call, 0, sizeof(*macro_call));
	
	macro_call->macro = macro;
	macro_call->argument_tokens = argument_tokens;
	macro_call->argument_index = SIZE_MAX;
	macro_call->argument_token_index = SIZE_MAX;

	return macro_call;
}

Token preprocessor_next_token(Preprocessor* state) {
	if (state->macro_call_stack_depth > 0) {
		// NOTE: Inside an expanding macro call.
		//       Instead of getting tokens from the tokenizer
		//       return the onces from the macro.
		//
		//       In that way the call gets replaced with whatever code was defined in the macro.

		Token token = {};
		if (preprocessor_get_next_macro_expantion_token(state, &token)) {
			return token;
		}
	}

	Token next_token = {};
	while (true) {
		next_token = tokenizer_next_token(&state->tokenizer);
		if (next_token.kind == TOKEN_HASH) {
			preprocessor_skip_derective(state);
		} else if (next_token.kind == TOKEN_IDENT) {
			const MacroDefinition* macro = macro_table_find(&state->macro_table, next_token.string);
			if (macro == NULL) {
				break;
			}

			MacroCallState* macro_call = preprocessor_init_macro_call(state, macro);
			if (macro_call == NULL) {
				continue;
			}

			Token token = {};
			bool has_first_token = preprocessor_get_next_macro_expantion_token(state, &token);
			assert(has_first_token);

			return token;
		} else {
			break;
		}
	}

	return next_token;
}
