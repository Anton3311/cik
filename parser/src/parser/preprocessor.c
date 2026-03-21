#include "preprocessor.h"

#include "parser/parse_tools.h"

void macro_table_append(MacroTable* table, const MacroDefinition* macro) {
	assert(table->count < table->capacity);

	table->macros[table->count] = *macro;
	table->count += 1;
}

bool macro_table_remove(MacroTable* table, String name) {
	for (size_t i = 0; i < table->count; i += 1) {
		if (str_equal(table->macros[i].name, name)) {
			table->macros[i] = table->macros[table->count - 1];
			table->count -= 1;
			return true;
		}
	}

	return false;
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

void _preprocessor_skip_until_newline(Preprocessor* state);

String directive_kind_to_string(DirectiveKind kind) {
	switch (kind) {
	case DIRECTIVE_INCLUDE:
		return STR_LIT("include");
	case DIRECTIVE_DEFINE:
		return STR_LIT("define");
	case DIRECTIVE_UNDEF:
		return STR_LIT("undef");
	case DIRECTIVE_IF:
		return STR_LIT("if");
	case DIRECTIVE_ELIF:
		return STR_LIT("elif");
	case DIRECTIVE_ELSE:
		return STR_LIT("else");
	case DIRECTIVE_ENDIF:
		return STR_LIT("endif");
	case DIRECTIVE_IFDEF:
		return STR_LIT("ifdef");
	case DIRECTIVE_IFNDEF:
		return STR_LIT("ifndef");
	case DIRECTIVE_PRAGMA:
		return STR_LIT("pragma");
	case DIRECTIVE_ERROR:
		return STR_LIT("error");
	}

	unreachable();
	return (String) {};
}

void preprocessor_init(Preprocessor* state,
		String source_path,
		String source_code,
		const LineInfo* line_info,
		Diagnostics* diagnostics,
		Arena* allocator,
		Arena* temp_allocator,
		Arena* generated_tokens_allocator) {
	state->allocator = allocator;
	state->temp_allocator = temp_allocator;
	state->generated_tokens_allocator = generated_tokens_allocator;

	state->source_path = source_path;
	state->line_info = *line_info;
	state->diagnostics = diagnostics;
	state->tokenizer = (Tokenizer) {
		.source_code = source_code,
		.read_position = 0,
	};

	state->macro_table = (MacroTable) {
		.capacity = 128,
		.count = 0,
	};

	state->macro_table.macros = arena_alloc_array(state->allocator, MacroDefinition, state->macro_table.capacity);

	{
		MacroDefinition line_macro = {
			.name = STR_LIT("__LINE__"),
			.builtin_kind = BUILTIN_MACRO_LINE,
			.token_count = 1,
		};

		macro_table_append(&state->macro_table, &line_macro);

		MacroDefinition file_macro = {
			.name = STR_LIT("__FILE__"),
			.builtin_kind = BUILTIN_MACRO_FILE,
			.token_count = 1,
		};

		macro_table_append(&state->macro_table, &file_macro);
	}

	state->macro_call_stack_capacity = 32;
	state->macro_call_stack_depth = 0;
	state->macro_call_stack = arena_alloc_array(state->allocator, MacroCallState, state->macro_call_stack_capacity);
}

void _preprocessor_parse_macro_token_stream(Preprocessor* state, MacroDefinition* macro) {
	macro->tokens = arena_alloc_array(state->allocator, Token, 0);

	ArenaRegion hints_temp_region = arena_begin_temp(state->temp_allocator);
	MacroTokenHint* token_hints = arena_alloc_array(state->temp_allocator, MacroTokenHint, 0);
	size_t token_hint_count = 0;

	SourceRange macro_name_range = source_range_from_sub_string(state->tokenizer.source_code, macro->name);
	uint32_t expected_token_line = line_info_pos_to_source_location(&state->line_info, macro_name_range.start).line;

	bool next_token_is_part_of_insert_operator = false;

	while (!tokenizer_is_end(&state->tokenizer)) {
		Token token = tokenizer_view_next(&state->tokenizer);
		uint32_t token_line = line_info_pos_to_source_location(&state->line_info, token.source_range.end).line;
		if (token_line == expected_token_line) {
			MacroTokenHint token_hint = { .kind = MACRO_TOKEN_HINT_NONE };

			if (token.kind == TOKEN_BACKWARD_SLASH) {
				expected_token_line += 1;
				tokenizer_reset_to_token(&state->tokenizer, token);
				continue;
			}

			switch (token.kind) {
			case TOKEN_IDENT:
				tokenizer_reset_to_token(&state->tokenizer, token);

				if (macro->style != MACRO_STYLE_FUNCTION) {
					break;
				}

				size_t parameter_index = macro_find_param_by_name(macro, token.string);

				bool is_insert_operator = false;
				if (next_token_is_part_of_insert_operator) {
					is_insert_operator = true;
				}

				Token maybe_token_insert_operator = tokenizer_view_next(&state->tokenizer);
				if (maybe_token_insert_operator.kind == TOKEN_DOUBLE_HASH) {
					tokenizer_reset_to_token(&state->tokenizer, maybe_token_insert_operator);
					is_insert_operator = true;
					next_token_is_part_of_insert_operator = true;
				} else if (parameter_index != SIZE_MAX) {
					token_hint.kind = MACRO_TOKEN_HINT_PARAMETER;
					token_hint.param.index = parameter_index;
				}

				if (is_insert_operator) {
					token_hint.kind = MACRO_TOKEN_HINT_TOKEN_INSERT_OPERATOR;
					token_hint.token_insert_op.param_index = parameter_index; // here an invalid param index is allowed
				}

				break;
			case TOKEN_HASH: {
				// Consume TOKEN_HASH, so that we can get the next identifier token,
				// without tokenizing TOKEN_HASH twice (since `tokenizer_view_next` was used).
				tokenizer_reset_to_token(&state->tokenizer, token);

				Token param_name_token = tokenizer_view_next(&state->tokenizer);
				if (param_name_token.kind != TOKEN_IDENT) {
					TokenKind expected_token = TOKEN_IDENT;

					diagnostics_report_unexpected_token(state->diagnostics,
							param_name_token,
							&expected_token,
							1);

					// NOTE: Terminate parsing here
					arena_end_temp(hints_temp_region);
					return;
				}

				tokenizer_reset_to_token(&state->tokenizer, param_name_token);

				size_t param_index = macro_find_param_by_name(macro, param_name_token.string);
				if (param_index == SIZE_MAX) {
					StringBuilder builder = { .arena = state->diagnostics->allocator };
					str_builder_append(&builder, STR_LIT("# must be followed by parameter name. "));
					str_builder_append_char(&builder, '\'');
					str_builder_append(&builder, param_name_token.string);
					str_builder_append(&builder, STR_LIT("' is not a valid macro parameter"));

					diagnostics_report_error(state->diagnostics,
							param_name_token.source_range,
							builder.string,
							NULL);

					// NOTE: Terminate parsing here
					arena_end_temp(hints_temp_region);
					return;
				}

				token_hint.kind = MACRO_TOKEN_HINT_STRING_OPERATOR;
				token_hint.string_op.param_index = param_index;

				// Initially token was a TOKEN_HASH. Replace it with the param name token,
				// so the tokenizer continues after the `param_name_token`.
				//
				// This has a side effect that the macro token stream, won't contain the hash token,
				// but only the param name token with `MACRO_TOKEN_HINT_STRING_OPERATOR`
				token = param_name_token;
				break;
			}
			case TOKEN_DOUBLE_HASH: {
				unreachable();
				break;
			}
			default:
				tokenizer_reset_to_token(&state->tokenizer, token);
				break;
			}

			if (macro->style != MACRO_STYLE_FUNCTION) {
				assert(token_hint.kind == MACRO_TOKEN_HINT_NONE);
			}

			if (macro->style == MACRO_STYLE_FUNCTION) {
				*arena_alloc(state->temp_allocator, MacroTokenHint) = token_hint;
				token_hint_count += 1;
			}

			// Token is on the same line as the macro definition,
			// so it belongs to the token stream of this macro
			*arena_alloc(state->allocator, Token) = token;
			macro->token_count += 1;
		} else {
			break;
		}
	}

	if (macro->style == MACRO_STYLE_FUNCTION) {
		assert(token_hint_count == macro->token_count);

		// Copy token hints, because they were allocated using the temporary allocator
		macro->token_hints = arena_alloc_array(state->allocator, MacroTokenHint, token_hint_count);
		memcpy(macro->token_hints, token_hints, sizeof(*token_hints) * token_hint_count);
	}

	arena_end_temp(hints_temp_region);
}

bool _preprocessor_parse_macro(Preprocessor* state, MacroDefinition* macro) {
	ArenaRegion temp_region = arena_begin_temp(state->allocator);

	Token name_token = tokenizer_next_token(&state->tokenizer);
	if (name_token.kind != TOKEN_IDENT) {
		diagnostics_report_error(state->diagnostics, name_token.source_range, STR_LIT("Expected macro name"), NULL);
		return false;
	}

	macro->style = MACRO_STYLE_DEFAULT;
	macro->name = name_token.string;

	Token maybe_paren = tokenizer_view_next(&state->tokenizer);

	// A function style is defined by a name followed by a left paren right after it (without any whitespace)
	if (maybe_paren.kind == TOKEN_LEFT_PAREN
			&& name_token.source_range.end == maybe_paren.source_range.start) {

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

const DirectiveKind INVALID_DIRECTIVE = -1;

DirectiveKind _directive_kind_from_string(String string) {
	if (str_equal(string, STR_LIT("include"))) {
		return DIRECTIVE_INCLUDE;
	} else if (str_equal(string, STR_LIT("define"))) {
		return DIRECTIVE_DEFINE;
	} else if (str_equal(string, STR_LIT("undef"))) {
		return DIRECTIVE_UNDEF;
	} else if (str_equal(string, STR_LIT("if"))) {
		return DIRECTIVE_IF;
	} else if (str_equal(string, STR_LIT("elif"))) {
		return DIRECTIVE_ELIF;
	} else if (str_equal(string, STR_LIT("else"))) {
		return DIRECTIVE_ELSE;
	} else if (str_equal(string, STR_LIT("endif"))) {
		return DIRECTIVE_ENDIF;
	} else if (str_equal(string, STR_LIT("ifdef"))) {
		return DIRECTIVE_IFDEF;
	} else if (str_equal(string, STR_LIT("ifndef"))) {
		return DIRECTIVE_IFNDEF;
	} else if (str_equal(string, STR_LIT("pragma"))) {
		return DIRECTIVE_PRAGMA;
	} else if (str_equal(string, STR_LIT("error"))) {
		return DIRECTIVE_ERROR;
	}

	return INVALID_DIRECTIVE;
}

typedef enum {
	EXPR_IDENT,
	EXPR_INT_LITERAL,
	EXPR_OP_DEFINED,
	EXPR_UNARY,
} ExprKind;

typedef enum {
	UNARY_NEGATE,
	UNARY_PLUS,

	UNARY_LOGICAL_NOT,
} UnaryOp;

typedef struct Expr Expr;
struct Expr {
	ExprKind kind;
	SourceRange source_range;

	union {
		struct {
			uint64_t value;
		} integer_literal;

		struct {
			Expr* macro;
		} op_defined;

		struct {
			UnaryOp op;
			Expr* operand;
		} unary;

		Token ident;
	};
};

Expr* _preprocessor_parse_expr(Preprocessor* state, Arena* allocator) {
	Token token = tokenizer_view_next(&state->tokenizer);

	if (token.kind == TOKEN_LEFT_PAREN) {
		tokenizer_reset_to_token(&state->tokenizer, token);

		Expr* expr = _preprocessor_parse_expr(state, allocator);

		Token closing_paren = tokenizer_next_token(&state->tokenizer);
		if (closing_paren.kind != TOKEN_RIGHT_PAREN) {
			TokenKind expected_tokens[] = { TOKEN_RIGHT_PAREN };
			diagnostics_report_unexpected_token(state->diagnostics,
					closing_paren,
					expected_tokens,
					array_size(expected_tokens));
			return NULL;
		}

		return expr;
	} else if (token.kind == TOKEN_EXCLAMATION_MARK || token.kind == TOKEN_MINUS || token.kind == TOKEN_PLUS) {
		tokenizer_reset_to_token(&state->tokenizer, token);

		UnaryOp op = -1;
		switch (token.kind) {
		case TOKEN_EXCLAMATION_MARK:
			op = UNARY_LOGICAL_NOT;
			break;
		case TOKEN_PLUS:
			op = UNARY_PLUS;
			break;
		case TOKEN_MINUS:
			op = UNARY_NEGATE;
			break;
		default:
			unreachable();
		}

		Expr* operand = _preprocessor_parse_expr(state, allocator);

		Expr* expr = arena_alloc(allocator, Expr);
		expr->kind = EXPR_UNARY;
		expr->unary.op = op;
		expr->unary.operand = operand;
		expr->source_range = (SourceRange) {
			.start = token.source_range.start,
			.end = operand->source_range.end,
		};
		return expr;
	} else if (token.kind == TOKEN_IDENT) {
		tokenizer_reset_to_token(&state->tokenizer, token);

		if (str_equal(token.string, STR_LIT("defined"))) {
			Expr* macro = _preprocessor_parse_expr(state, allocator);
			if (!macro) {
				return NULL;
			}

			Expr* expr = arena_alloc(allocator, Expr);
			expr->kind = EXPR_OP_DEFINED;
			expr->source_range = (SourceRange) {
				.start = token.source_range.start,
				.end = macro->source_range.end,
			};
			expr->op_defined.macro = macro;
			return expr;
		} else if (is_digit(token.string.v[0])) {
			uint64_t value = 0;
			bool result = parse_integer_literal(state->diagnostics, token, &value);

			if (!result) {
				return NULL;
			}

			Expr* expr = arena_alloc(allocator, Expr);
			expr->kind = EXPR_INT_LITERAL;
			expr->integer_literal.value = value;

			return expr;
		} else {
			Expr* expr = arena_alloc(allocator, Expr);
			expr->kind = EXPR_IDENT;
			expr->ident = token;
			expr->source_range = token.source_range;
			return expr;
		}
	}

	return NULL;
}

bool _expr_to_boolean(Preprocessor* state, Expr* expr) {
	assert(expr);

	switch (expr->kind) {
	case EXPR_IDENT:
		break;
	case EXPR_INT_LITERAL:
		return expr->integer_literal.value > 0;
	case EXPR_UNARY: {
		bool result = _expr_to_boolean(state, expr->unary.operand);

		switch (expr->unary.op) {
		case UNARY_PLUS:
		case UNARY_NEGATE:
			assert_msg(false, "todo");
		case UNARY_LOGICAL_NOT:
			return !result;
		}

		unreachable();
	}
	case EXPR_OP_DEFINED: {
		assert(expr->op_defined.macro->kind == EXPR_IDENT);
		const MacroDefinition* macro = macro_table_find(&state->macro_table, expr->op_defined.macro->ident.string);
		return macro != NULL;
	}
	}
	
	unreachable();
	return false;
}

bool _preprocessor_parse_condition(Preprocessor* state, bool* out_result) {
	ArenaRegion temp = arena_begin_temp(state->temp_allocator);
	Expr* expr = _preprocessor_parse_expr(state, state->temp_allocator);

	if (!expr) {
		arena_end_temp(temp);
		return false;
	}

	*out_result = _expr_to_boolean(state, expr);

	arena_end_temp(temp);
	return true;
}

bool _preprocessor_is_current_region_enabled(Preprocessor* state) {
	return state->current_branch_state == NULL
		|| (state->current_branch_state && state->current_branch_state->predicate_value);
}

bool _preprocessor_parse_directive(Preprocessor* state, ParsedDirective directive) {
	switch (directive.kind) {
	case DIRECTIVE_INCLUDE: {
		tokenizer_skip_whitespace_and_comments(&state->tokenizer);

		char opening_quote = state->tokenizer.source_code.v[state->tokenizer.read_position];
		if (opening_quote == '<') {
			Token string_token = {};
			StringTokenizerResult result = _tokenizer_try_create_string_token(&state->tokenizer, '<', '>', &string_token);
			assert(result == STR_TOKEN_RESULT_NONE);
		} else if (opening_quote == '"') {
			Token string_token = {};
			StringTokenizerResult result = _tokenizer_try_create_string_token(&state->tokenizer, '"', '"', &string_token);
			assert(result == STR_TOKEN_RESULT_NONE);
		} else {
			Token next_token = tokenizer_next_token(&state->tokenizer);
			diagnostics_report_error(state->diagnostics,
					next_token.source_range,
					STR_LIT("Expected include path"),
					NULL);
			return false;
		}
		break;
	}
	case DIRECTIVE_PRAGMA: {
		_preprocessor_skip_until_newline(state);
		return true;
	}
	case DIRECTIVE_DEFINE: {
		MacroDefinition macro = {};
		if (_preprocessor_parse_macro(state, &macro)) {
			if (_preprocessor_is_current_region_enabled(state)) {
				macro_table_append(&state->macro_table, &macro);
			}
		}
		break;
	}
	case DIRECTIVE_UNDEF: {
		Token macro_name = tokenizer_next_token(&state->tokenizer);
		if (_preprocessor_is_current_region_enabled(state)) {
			if (macro_name.kind != TOKEN_IDENT) {
				TokenKind expected_tokens[] = { TOKEN_IDENT };
				diagnostics_report_unexpected_token(state->diagnostics,
						macro_name,
						expected_tokens,
						array_size(expected_tokens));
				return false;
			}

			bool result = macro_table_remove(&state->macro_table, macro_name.string);
			if (result) {
				debug_log_info("undef %.*s", STR_FMT(macro_name.string));
			} else {
				debug_log_warn("undef %.*s failed", STR_FMT(macro_name.string));
			}
		}
		break;
	}
	case DIRECTIVE_IF: {
		PreprocessorBranchState* branch_state = arena_alloc(state->allocator, PreprocessorBranchState);
		branch_state->parent = state->current_branch_state;
		branch_state->current_directive = directive;

		bool predicate_value = false;
		if (!_preprocessor_parse_condition(state, &predicate_value)) {
			return false;
		}

		branch_state->has_enabled_alternative_branch = predicate_value;

		if (branch_state->parent) {
			branch_state->predicate_value = predicate_value && branch_state->parent->predicate_value;
		} else {
			branch_state->predicate_value = predicate_value;
		}

		state->current_branch_state = branch_state;
		break;
	}
	case DIRECTIVE_IFDEF:
	case DIRECTIVE_IFNDEF: {
		Token macro_name = tokenizer_next_token(&state->tokenizer);
		if (macro_name.kind != TOKEN_IDENT) {
			TokenKind expected_tokens[] = { TOKEN_IDENT };
			diagnostics_report_unexpected_token(state->diagnostics,
					macro_name,
					expected_tokens,
					array_size(expected_tokens));
			return false;
		}

		PreprocessorBranchState* branch_state = arena_alloc(state->allocator, PreprocessorBranchState);
		branch_state->parent = state->current_branch_state;
		branch_state->current_directive = directive;
		
		bool flip_predicate = directive.kind == DIRECTIVE_IFNDEF;
		bool is_macro_defined = macro_table_find(&state->macro_table, macro_name.string) != NULL;
		bool predicate_value = is_macro_defined ^ flip_predicate;

		branch_state->has_enabled_alternative_branch = predicate_value;

		if (branch_state->parent) {
			branch_state->predicate_value = predicate_value && branch_state->parent->predicate_value;
		} else {
			branch_state->predicate_value = predicate_value;
		}

		state->current_branch_state = branch_state;
		break;
	}
	case DIRECTIVE_ELSE: {
		PreprocessorBranchState* branch_state = state->current_branch_state;
		if (branch_state == NULL) {
			diagnostics_report_error(state->diagnostics,
					directive.source_range,
					STR_LIT("#else has no matching #if directive"),
					NULL);
			return false;
		}

		switch (branch_state->current_directive.kind) {
		case DIRECTIVE_IF:
		case DIRECTIVE_IFDEF:
		case DIRECTIVE_IFNDEF:
		case DIRECTIVE_ELIF:
			break;
		default: {
			StringBuilder builder = { state->diagnostics->allocator };
			str_builder_append(&builder, STR_LIT("#elif directive can't appear after "));
			str_builder_append(&builder, directive_kind_to_string(branch_state->current_directive.kind));

			DiagnosticsEntry* error = diagnostics_report_error(state->diagnostics,
					directive.source_range,
					builder.string,
					NULL);

			diagnostics_report_error(state->diagnostics,
					branch_state->current_directive.source_range,
					STR_LIT("Previous directive here"),
					error);
			return false;
		}
		}

		if (branch_state->parent) {
			branch_state->predicate_value = !branch_state->has_enabled_alternative_branch
				&& !branch_state->parent->predicate_value;
		} else {
			branch_state->predicate_value = !branch_state->has_enabled_alternative_branch;
		}

		branch_state->current_directive = directive;
		break;
	}
	case DIRECTIVE_ELIF: {
		PreprocessorBranchState* branch_state = state->current_branch_state;
		if (branch_state == NULL) {
			diagnostics_report_error(state->diagnostics,
					directive.source_range,
					STR_LIT("#elif has no matching #if directive"),
					NULL);
			return false;
		}

		switch (branch_state->current_directive.kind) {
		case DIRECTIVE_IF:
		case DIRECTIVE_IFDEF:
		case DIRECTIVE_IFNDEF:
		case DIRECTIVE_ELIF:
			break;
		default: {
			StringBuilder builder = { state->diagnostics->allocator };
			str_builder_append(&builder, STR_LIT("#elif directive can't appear after "));
			str_builder_append(&builder, directive_kind_to_string(branch_state->current_directive.kind));

			DiagnosticsEntry* error = diagnostics_report_error(state->diagnostics,
					directive.source_range,
					builder.string,
					NULL);

			diagnostics_report_error(state->diagnostics,
					branch_state->current_directive.source_range,
					STR_LIT("Previous directive here"),
					error);
			return false;
		}
		}

		bool predicate_value = false;
		if (!_preprocessor_parse_condition(state, &predicate_value)) {
			return false;
		}

		branch_state->has_enabled_alternative_branch = predicate_value;

		branch_state->predicate_value = !branch_state->predicate_value && predicate_value;
		branch_state->current_directive = directive;
		break;
	}
	case DIRECTIVE_ENDIF: {
		if (state->current_branch_state == NULL) {
			diagnostics_report_error(state->diagnostics,
					directive.source_range,
					STR_LIT("#endif used without #if"),
					NULL);
			return false;
		}
		state->current_branch_state = state->current_branch_state->parent;
		break;
	}
	case DIRECTIVE_ERROR: {
		uint32_t initial_line = line_info_pos_to_source_location(&state->line_info, directive.source_range.end).line;

		Token first_token = { .kind = TOKEN_COUNT };
		Token last_token = { .kind = TOKEN_COUNT };

		while (true) {
			Token token = tokenizer_view_next(&state->tokenizer);
			
			uint32_t token_line = line_info_pos_to_source_location(&state->line_info, token.source_range.end).line; 

			if (token_line > initial_line) {
				break;
			} else if (token.kind == TOKEN_EOF) {
				// Stop only if the EOF token is on this line
				tokenizer_reset_to_token(&state->tokenizer, token);
				break;
			} else {
				tokenizer_reset_to_token(&state->tokenizer, token);

				if (first_token.kind == TOKEN_COUNT) {
					first_token = token;
					last_token = token;
				} else {
					last_token = token;
				}
			}
		}

		if (_preprocessor_is_current_region_enabled(state)) {
			SourceRange error_message_range = (SourceRange) {
				.start = first_token.source_range.start,
				.end = last_token.source_range.end,
			};

			String error_message = sub_str(state->tokenizer.source_code,
					error_message_range.start,
					error_message_range.end - error_message_range.start);

			diagnostics_report_error(state->diagnostics,
					directive.source_range,
					error_message,
					NULL);
		}
		return false;
	}
	}

	return true;
}

bool _preprocessor_parse_directive_statement(Preprocessor* state, ParsedDirective* out_directive) {
	assert(out_directive != NULL);

	Token hash_token = tokenizer_next_token(&state->tokenizer);
	assert(hash_token.kind == TOKEN_HASH);

	Token directive_name_token = tokenizer_next_token(&state->tokenizer);
	if (directive_name_token.kind != TOKEN_IDENT) {
		TokenKind expected_tokens[] = { TOKEN_IDENT };
		diagnostics_report_unexpected_token(state->diagnostics,
				directive_name_token,
				expected_tokens,
				array_size(expected_tokens));
		return false;
	}

	DirectiveKind directive_kind = _directive_kind_from_string(directive_name_token.string);
	if (directive_kind == INVALID_DIRECTIVE) {
		StringBuilder builder = { .arena = state->diagnostics->allocator };
		str_builder_append(&builder, STR_LIT("Unknown preprocessor directive type: "));
		str_builder_append(&builder, directive_name_token.string);

		diagnostics_report_error(state->diagnostics,
				directive_name_token.source_range,
				builder.string,
				NULL);
		return false;
	}

	out_directive->kind = directive_kind;
	out_directive->source_range = (SourceRange) {
		.start = hash_token.source_range.start,
		.end = directive_name_token.source_range.end,
	};
	return true;
}

void _preprocessor_skip_until_newline(Preprocessor* state) {
	uint32_t initial_line = line_info_pos_to_source_location(&state->line_info, state->tokenizer.read_position).line;

	while (true) {
		Token token = tokenizer_view_next(&state->tokenizer);
		uint32_t token_line = line_info_pos_to_source_location(&state->line_info, token.source_range.end).line; 

		if (token_line > initial_line) {
			break;
		} else if (token.kind == TOKEN_EOF) {
			// Stop only if the EOF token is on this line
			tokenizer_reset_to_token(&state->tokenizer, token);
			break;
		} else {
			tokenizer_reset_to_token(&state->tokenizer, token);
		}
	}
}

void preprocessor_skip_directive(Preprocessor* state) {
	ParsedDirective directive = {};
	if (!_preprocessor_parse_directive_statement(state, &directive)) {
		_preprocessor_skip_until_newline(state);
		return;
	}

	if (!_preprocessor_parse_directive(state, directive)) {
		_preprocessor_skip_until_newline(state);
		return;
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

// Merges subsequent tokens with `MACRO_TOKEN_HINT_TOKEN_INSERT_OPERATOR` hints into a single identifier token.
//
// NOTE: Supports merging only if each of the macro arguments has exactly one token.
bool _preprocessor_apply_token_insert_operator(Preprocessor* state, MacroCallState* macro_call, Token* out_token) {
	const MacroDefinition* macro = macro_call->macro;
	assert(macro->style == MACRO_STYLE_FUNCTION);

	StringBuilder builder = { .arena = state->generated_tokens_allocator };
	SourceRange source_range = { SIZE_MAX, SIZE_MAX };

	size_t token_count = macro->token_count;
	while (macro_call->token_index < token_count) {
		const Token token = macro->tokens[macro_call->token_index];
		const MacroTokenHint hint = macro->token_hints[macro_call->token_index];

		if (hint.kind != MACRO_TOKEN_HINT_TOKEN_INSERT_OPERATOR) {
			break;
		}

		if (source_range.start == SIZE_MAX) {
			source_range = token.source_range;
		} else {
			source_range.end = token.source_range.end;
		}

		size_t param_index = hint.token_insert_op.param_index;
		if (param_index == SIZE_MAX) {
			str_builder_append(&builder, token.string);
		} else {
			assert(param_index < macro->parameter_count);

			const MacroArgumentTokens argument_tokens = macro_call->argument_tokens[param_index];
			assert(argument_tokens.count == 1);
			str_builder_append(&builder, argument_tokens.tokens[0].string);
		}

		macro_call->token_index += 1;
	}

	Token token = (Token) {
		.kind = TOKEN_IDENT,
		.source_range = source_range,
		.string = builder.string,
	};

	*out_token = token;
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
		const MacroDefinition* macro = macro_call->macro;
		switch (macro->builtin_kind) {
		case BUILTIN_MACRO_NONE: {
			Token next_token = macro_call->macro->tokens[macro_call->token_index];

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
				macro_call->argument_index = hint.param.index;
				macro_call->argument_token_index = 0;

				bool has_first_token = _preprocessor_try_expand_macro_argument(state, out_token);
				assert_msg(has_first_token, "Macro call argument does has 0 tokens");
				return true;
			}
			case MACRO_TOKEN_HINT_STRING_OPERATOR: {
				StringBuilder builder = { .arena = state->generated_tokens_allocator };
				str_builder_append_char(&builder, '"');

				assert(hint.string_op.param_index < macro->parameter_count);

				MacroArgumentTokens argument_tokens = macro_call->argument_tokens[hint.string_op.param_index];

				if (argument_tokens.count > 0) {
					str_builder_append(&builder, argument_tokens.tokens[0].string);
				}

				for (size_t i = 1; i < argument_tokens.count; i += 1) {
					str_builder_append_char(&builder, ' ');
					str_builder_append(&builder, argument_tokens.tokens[i].string);
				}

				str_builder_append_char(&builder, '"');

				Token token = (Token) {
					.kind = TOKEN_STRING,
					.source_range = next_token.source_range,
					.string = builder.string,
				};

				// We've precessed the string operator token => go to the next one in the token stream of the macro.
				macro_call->token_index += 1;

				*out_token = token;
				return true;
			}
			case MACRO_TOKEN_HINT_TOKEN_INSERT_OPERATOR:
				return _preprocessor_apply_token_insert_operator(state, macro_call, out_token);
			}
			break;
		}
		case BUILTIN_MACRO_LINE: {
			assert(state->macro_call_stack_depth > 0);
			SourceRange first_call_range = state->macro_call_stack[0].call_source_range;
			uint32_t call_source_line = line_info_pos_to_source_location(
					&state->line_info,
					first_call_range.start).line + 1;

			StringBuilder builder = { .arena = state->generated_tokens_allocator };
			str_builder_append_int(&builder, call_source_line);

			Token generated_token = (Token) {
				.kind = TOKEN_IDENT,
				.source_range = first_call_range,
				.string = builder.string,
			};

			// Macro call finished
			macro_call->token_index = macro->token_count;

			*out_token = generated_token;
			return true;
		}
		case BUILTIN_MACRO_FILE: {
			assert(state->macro_call_stack_depth > 0);
			SourceRange first_call_range = state->macro_call_stack[0].call_source_range;

			StringBuilder builder = { .arena = state->generated_tokens_allocator };
			str_builder_append_char(&builder, '"');
			str_builder_append(&builder, state->source_path);
			str_builder_append_char(&builder, '"');

			Token generated_token = (Token) {
				.kind = TOKEN_STRING,
				.source_range = first_call_range,
				.string = builder.string,
			};

			// Mark the call as finished
			macro_call->token_index = macro->token_count;

			*out_token = generated_token;
			return true;
		}
		}

	}

	return false;
}

// Returns the next token from the macro call expantion
// (Uses `preprocessor_get_next_macro_expantion_token` for that).
//
// In case `preprocessor_get_next_macro_expantion_token` fails
// (when the preprocessor isn't expanding a macro)
// falls back to the source tokenizer.
Token _preprocessor_next_macro_or_source_token(Preprocessor* state) {
	Token token = {};
	if (state->macro_call_stack_depth > 0 && preprocessor_get_next_macro_expantion_token(state, &token)) {
		return token;
	}

	return tokenizer_next_token(&state->tokenizer);
}

void _preprocessor_macro_call_to_diagnostics(const Preprocessor* state,
		size_t call_index,
		DiagnosticsEntry* root_error) {
	assert(root_error != NULL);
	assert(call_index < state->macro_call_stack_depth);

	const MacroCallState* call = &state->macro_call_stack[call_index];
	const MacroDefinition* macro = call->macro;

	StringBuilder builder = { .arena = state->diagnostics->allocator };

	str_builder_append(&builder, STR_LIT("Expended from macro '"));
	str_builder_append(&builder, call->macro->name);
	str_builder_append(&builder, STR_LIT("'\n"));

	for (size_t param_index = 0; param_index < macro->parameter_count; param_index += 1) {
		str_builder_append_char(&builder, '\t');
		str_builder_append(&builder, macro->parameter_names[param_index]);
		str_builder_append(&builder, STR_LIT(" = "));

		for (size_t token_index = 0; token_index < call->argument_tokens[param_index].count; token_index += 1) {
			Token token = call->argument_tokens[param_index].tokens[token_index];
			str_builder_append(&builder, token.string);
			str_builder_append_char(&builder, ' ');
		}

		str_builder_append_char(&builder, '\n');
	}

	diagnostics_report_error(state->diagnostics,
			call->call_source_range,
			builder.string,
			root_error);
}

void _preprocessor_macro_call_stack_to_diagnostics(const Preprocessor* state, DiagnosticsEntry* root_error) {
	for (size_t call_index = 0; call_index < state->macro_call_stack_depth; call_index += 1) {
		_preprocessor_macro_call_to_diagnostics(state, call_index, root_error);
	}
}

MacroCallState* preprocessor_init_macro_call(Preprocessor* state, const MacroDefinition* macro, Token macro_call_ident) {
	MacroArgumentTokens* argument_tokens = NULL;

	SourceRange call_source_range = macro_call_ident.source_range;

	if (macro->style == MACRO_STYLE_FUNCTION) {
		// Parse macro arguments
		Token maybe_left_paren = _preprocessor_next_macro_or_source_token(state);
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
				Token token = _preprocessor_next_macro_or_source_token(state);
				if (token.kind == TOKEN_COMMA) {
					break;
				} else if (token.kind == TOKEN_RIGHT_PAREN) {
					end_argument_list = true;
					call_source_range.end = token.source_range.end;
					break;
				}

				arena_alloc(state->temp_allocator, Token);

				tokens->tokens[tokens->count] = token;
				tokens->count += 1;
			}

			if (end_argument_list) {
				if (arg_index != macro->parameter_count - 1) {
					// Not enough macro arguments
					StringBuilder builder = { state->diagnostics->allocator };
					str_builder_append(&builder, STR_LIT("Not enough arguments during a call of macro called '"));
					str_builder_append(&builder, macro->name);
					str_builder_append(&builder, STR_LIT("'. Expected "));
					str_builder_append_int(&builder, macro->parameter_count);
					str_builder_append(&builder, STR_LIT(" but only "));
					str_builder_append_int(&builder, arg_index + 1);
					str_builder_append(&builder, STR_LIT(" were provided."));

					DiagnosticsEntry* error = diagnostics_report_error(state->diagnostics,
							source_range_from_sub_string(state->tokenizer.source_code, macro->name),
							builder.string,
							NULL);

					_preprocessor_macro_call_stack_to_diagnostics(state, error);
					return NULL;
				}

				break;
			}
		}

		if (!end_argument_list) {
			Token maybe_right_paren = _preprocessor_next_macro_or_source_token(state);
			if (maybe_right_paren.kind != TOKEN_RIGHT_PAREN) {
				TokenKind expected_tokens[] = { TOKEN_RIGHT_PAREN };
				diagnostics_report_unexpected_token(state->diagnostics,
						maybe_right_paren,
						expected_tokens,
						array_size(expected_tokens));
				return NULL;
			}

			call_source_range.end = maybe_right_paren.source_range.end;
		}
	}

	if (macro->token_count == 0) {
		return NULL;
	}

	if (state->macro_call_stack_depth >= state->macro_call_stack_capacity) {
		DiagnosticsEntry* overflow_error = diagnostics_report_error(state->diagnostics,
				call_source_range,
				STR_LIT("Macro callstack overflow"),
				NULL);

		_preprocessor_macro_call_stack_to_diagnostics(state, overflow_error);
		return NULL;
	}

	MacroCallState* macro_call = &state->macro_call_stack[state->macro_call_stack_depth];
	state->macro_call_stack_depth += 1;

	memset(macro_call, 0, sizeof(*macro_call));
	
	macro_call->macro = macro;
	macro_call->call_source_range = call_source_range;
	macro_call->argument_tokens = argument_tokens;
	macro_call->argument_index = SIZE_MAX;
	macro_call->argument_token_index = SIZE_MAX;

	return macro_call;
}

Token preprocessor_view_next(Preprocessor* state) {
	if (state->has_pending_next_token) {
		return state->pending_next_token;
	}

	Token next_token = preprocessor_next_token(state);
	state->pending_next_token = next_token;
	state->has_pending_next_token = true;
	return next_token;
}

Token preprocessor_next_token(Preprocessor* state) {
	if (state->has_pending_next_token) {
		state->has_pending_next_token = false;

		Token token = state->pending_next_token;
		state->pending_next_token = (Token) {};
		return token;
	}

	while (true) {
		if (state->current_branch_state != NULL && !state->current_branch_state->predicate_value) {
			Token token = tokenizer_view_next(&state->tokenizer);
			switch (token.kind) {
			case TOKEN_HASH:
				preprocessor_skip_directive(state);
				break;
			case TOKEN_EOF:
				return token;
			default:
				tokenizer_reset_to_token(&state->tokenizer, token);
			}

			continue;
		}

		Token next_token = { .kind = TOKEN_COUNT };
		if (state->macro_call_stack_depth > 0) {
			// NOTE: Inside an expanding macro call.
			//       Instead of getting tokens from the tokenizer
			//       return the onces from the macro.
			//
			//       In that way the call gets replaced with whatever code was defined in the macro.

			preprocessor_get_next_macro_expantion_token(state, &next_token);
		}
		
		if (next_token.kind == TOKEN_COUNT) {
			next_token = tokenizer_view_next(&state->tokenizer);

			if (next_token.kind == TOKEN_HASH) {
				preprocessor_skip_directive(state);
				continue;
			} else {
				tokenizer_reset_to_token(&state->tokenizer, next_token);
			}
		}

		if (next_token.kind == TOKEN_IDENT) {
			const MacroDefinition* macro = macro_table_find(&state->macro_table, next_token.string);
			if (macro == NULL) {
				return next_token;
			}

			preprocessor_init_macro_call(state, macro, next_token);

			// NOTE: Possible cases:
			//       1. The macro call started successfully, because the it produces tokens.
			//       2. The macro call was skipped, since it doesn't produce any tokens.
			//       3. Macro call failed.
			//
			//       In the first case continueing goes to start of the loop and
			//       retreives the first token from the macro call.
			//
			//       In the second case it will find and process the next token
			//       after the macro call.
			continue;
		} else {
			return next_token;
		}
	}

	unreachable();
	return (Token) {};
}
