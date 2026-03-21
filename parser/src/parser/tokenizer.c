#include "tokenizer.h"

#include <ctype.h>

static String s_token_kind_to_string[TOKEN_COUNT] = {
	[TOKEN_EOF] = STR_LIT("<eof>"),

	[TOKEN_IDENT] = STR_LIT("<identifier>"),
	[TOKEN_STRING] = STR_LIT("<string>"),

	[TOKEN_HASH] = STR_LIT("#"),
	[TOKEN_COMMA] = STR_LIT(","),
	[TOKEN_DOT] = STR_LIT("."),
	[TOKEN_COLON] = STR_LIT(":"),
	[TOKEN_SEMICOLON] = STR_LIT(";"),
	[TOKEN_AMPERSAND] = STR_LIT("&"),
	[TOKEN_PIPE] = STR_LIT("|"),
	[TOKEN_EXCLAMATION_MARK] = STR_LIT("!"),
	[TOKEN_QUESTION_MARK] = STR_LIT("?"),
	[TOKEN_FORWARD_SLASH] = STR_LIT("/"),
	[TOKEN_ARROW] = STR_LIT("->"),

	// Parens & friends
	[TOKEN_LEFT_PAREN] = STR_LIT("("),
	[TOKEN_RIGHT_PAREN] = STR_LIT(")"),

	[TOKEN_LEFT_BRACE] = STR_LIT("{"),
	[TOKEN_RIGHT_BRACE] = STR_LIT("}"),

	[TOKEN_LEFT_BRACKET] = STR_LIT("["),
	[TOKEN_RIGHT_BRACKET] = STR_LIT("]"),

	// Arithmetics
	[TOKEN_PLUS] = STR_LIT("+"),
	[TOKEN_MINUS] = STR_LIT("-"),
	[TOKEN_ASTERISK] = STR_LIT("*"),
	[TOKEN_PERCENT] = STR_LIT("%"),

	[TOKEN_DOUBLE_PLUS] = STR_LIT("++"),
	[TOKEN_DOUBLE_MINUS] = STR_LIT("--"),

	[TOKEN_ASSIGNMENT_BY_SUM] = STR_LIT("+="),
	[TOKEN_ASSIGNMENT_BY_DIFFERENCE] = STR_LIT("-="),
	[TOKEN_ASSIGNMENT_BY_PRODUCT] = STR_LIT("*="),
	[TOKEN_ASSIGNMENT_BY_QUOTIENT] = STR_LIT("/="),
	[TOKEN_ASSIGNMENT_BY_REMAINDER] = STR_LIT("%="),

	[TOKEN_ASSIGNMENT_BY_BITWISE_AND] = STR_LIT("&="),
	[TOKEN_ASSIGNMENT_BY_BITWISE_OR] = STR_LIT("|="),
	[TOKEN_ASSIGNMENT_BY_BITWISE_XOR] = STR_LIT("^="),

	// Comparison
	[TOKEN_LESS] = STR_LIT("<"),
	[TOKEN_GREATER] = STR_LIT(">"),
	[TOKEN_LESS_OR_EQUAL] = STR_LIT("<="),
	[TOKEN_GREATER_OR_EQUAL] = STR_LIT(">="),
	[TOKEN_EQUAL] = STR_LIT("="),
	[TOKEN_NOT_EQUAL] = STR_LIT("!="),
	[TOKEN_DOUBLE_EQUAL] = STR_LIT("=="),

	// Logic
	[TOKEN_LOGIC_AND] = STR_LIT("&&"),
	[TOKEN_LOGIC_OR] = STR_LIT("||"),

	// Bitwise
	[TOKEN_BITWISE_XOR] = STR_LIT("^"),
	[TOKEN_BITWISE_NOT] = STR_LIT("~"),

	// Keywords
	[TOKEN_KEYWORD_TYPEDEF] = STR_LIT("typedef"),
	[TOKEN_KEYWORD_STRUCT] = STR_LIT("struct"),
	[TOKEN_KEYWORD_ENUM] = STR_LIT("enum"),
	[TOKEN_KEYWORD_CONST] = STR_LIT("const"),
};

String token_kind_to_string(TokenKind kind) {
	return s_token_kind_to_string[kind];
}

bool _tokenizer_try_skip_comment(Tokenizer* tokenizer);

inline Token _tokenizer_create_single_char_token(Tokenizer* tokenizer, TokenKind kind) {
	size_t position = tokenizer->read_position;
	Token token = (Token) {
		.source_range = (SourceRange) {
			.start = position,
			.end = position + 1,
		},
		.string = sub_str(tokenizer->source_code, position, 1),
		.kind = kind,
	};

	tokenizer->read_position += 1;
	return token;
}

bool _tokenizer_try_create_ident_token(Tokenizer* tokenizer, Token* out_token) {
	size_t token_start = tokenizer->read_position;

	while (true) {
		if (tokenizer_is_end(tokenizer)) {
			break;
		}

		char32_t current_char = tokenizer_get_char(tokenizer);
		if ((current_char >= 'a' && current_char <= 'z')
				|| (current_char >= 'A' && current_char <= 'Z')
				|| (current_char >= '0' && current_char <= '9')
				|| current_char == '_') {
			tokenizer->read_position += 1;
		} else {
			break;
		}
	}

	size_t string_length = tokenizer->read_position - token_start;
	if (string_length == 0) {
		return false;
	}

	String token_string = sub_str(tokenizer->source_code, token_start, string_length);
	TokenKind token_kind = TOKEN_IDENT;
	if (str_equal(token_string, STR_LIT("typedef"))) {
		token_kind = TOKEN_KEYWORD_TYPEDEF;
	} else if (str_equal(token_string, STR_LIT("struct"))) {
		token_kind = TOKEN_KEYWORD_STRUCT;
	} else if (str_equal(token_string, STR_LIT("enum"))) {
		token_kind = TOKEN_KEYWORD_ENUM;
	} else if (str_equal(token_string, STR_LIT("const"))) {
		token_kind = TOKEN_KEYWORD_CONST;
	}

	*out_token = (Token) {
		.source_range = (SourceRange) {
			.start = token_start,
			.end = tokenizer->read_position,
		},
		.string = token_string,
		.kind = token_kind,
	};
	return true;
}

void tokenizer_skip_whitespace_and_comments(Tokenizer* tokenizer) {
	while (true) {
		if (tokenizer_is_end(tokenizer)) {
			break;
		}

		while (true) {
			bool skipped = _tokenizer_try_skip_comment(tokenizer);
			if (!skipped) {
				break;
			}
		}

		char32_t current_char = tokenizer_get_char(tokenizer);
		if (!isspace(current_char)) {
			break;
		} else {
			tokenizer->read_position += 1;
		}
	}
}

// NOTE: Returnd token includes quotation marks
StringTokenizerResult _tokenizer_try_create_string_token(Tokenizer* tokenizer,
		char32_t string_opening_char,
		char32_t string_closing_char,
		Token* out_token) {
	size_t string_start = tokenizer->read_position;
	char32_t opening_char = tokenizer_get_char(tokenizer);

	// NOTE: This shouldn't be triggerable by the user code,
	//       only by missuse of this function.
	assert(opening_char == string_opening_char);
	tokenizer->read_position += 1;

	while (true) {
		if (tokenizer_is_end(tokenizer)) {
			return STR_TOKEN_RESULT_EOF_REACHED;
		}

		char32_t current_char = tokenizer_get_char(tokenizer);
		if (current_char == '\\') {
			tokenizer->read_position += 1;
			if (tokenizer_is_end(tokenizer)) {
				return STR_TOKEN_RESULT_EOF_REACHED;
			}

			char32_t escaped_char = tokenizer_get_char(tokenizer);
			switch (escaped_char) {
			case '\'':
			case '\"':
			case '\\':
			case 'n':
			case 'r':
			case 't':
			case '0':
				break;
			default:
				return STR_TOKEN_RESULT_INVALID_ESCAPE_CHAR;
			}

			// consume escaped char
			tokenizer->read_position += 1;
			continue;
		}

		if (current_char == string_closing_char) {
			tokenizer->read_position += 1;

			size_t string_length = tokenizer->read_position - string_start;
			*out_token = (Token) {
				.source_range = (SourceRange) {
					.start = string_start,
					.end = tokenizer->read_position,
				},
				.string = sub_str(tokenizer->source_code, string_start, string_length),
				.kind = TOKEN_STRING,
			};

			return STR_TOKEN_RESULT_NONE;
		}

		if (current_char == '\n') {
			return STR_TOKEN_RESULT_NEWLINE_REACHED;
		}

		tokenizer->read_position += 1;
	}

	return STR_TOKEN_RESULT_NONE;
}

Token _tokenizer_try_create_double_char_token(Tokenizer* tokenizer,
		char32_t first_char,
		char32_t second_char,
		TokenKind single_type,
		TokenKind double_type) {
	size_t token_start = tokenizer->read_position;
	char32_t c = tokenizer_get_char(tokenizer);
	tokenizer->read_position += 1;
	assert(c == first_char);

	if (!tokenizer_is_end(tokenizer)) {
		char32_t c2 = tokenizer_get_char(tokenizer);
		if (c2 == second_char) {
			tokenizer->read_position += 1;
			return (Token) {
				.source_range = (SourceRange) {
					.start = token_start,
					.end = tokenizer->read_position,
				},
				.string = sub_str(tokenizer->source_code, token_start, tokenizer->read_position - token_start),
				.kind = double_type,
			};
		}
	}

	return (Token) {
		.source_range = (SourceRange) {
			.start = token_start,
			.end = tokenizer->read_position,
		},
		.string = sub_str(tokenizer->source_code, token_start, tokenizer->read_position - token_start),
		.kind = single_type,
	};
}

/*
   Skips both single-line and multi-line comments.
*/
bool _tokenizer_try_skip_comment(Tokenizer* tokenizer) {
	char32_t current_char = tokenizer_get_char(tokenizer);
	if (current_char != '/') {
		return false;
	}

	bool is_single_line_comment = _tokenizer_has_next_char(tokenizer, '/');
	bool is_multi_line_comment = _tokenizer_has_next_char(tokenizer, '*');
	
	bool is_comment = is_single_line_comment || is_multi_line_comment;
	if (!is_comment) {
		return false;
	}

	tokenizer->read_position += 1; // consume '/'

	if (is_single_line_comment) {
		tokenizer->read_position += 1; // consume '/'

		// Single line comments ends at newline
		while (!tokenizer_is_end(tokenizer)) {
			char32_t c = tokenizer_get_char(tokenizer);
			if (c == '\n') {
				tokenizer->read_position += 1; // consume newline
				return true;
			} else {
				tokenizer->read_position += 1;
			}
		}
	} else if (is_multi_line_comment) {
		tokenizer->read_position += 1; // consume '*'
		while (true) {
			if (tokenizer_is_end(tokenizer)) {
				// NOTE: EOF reached, but the comment wasn't terminated
				break;
			}

			char32_t c = tokenizer_get_char(tokenizer);
			if (c == '*') {
				if (_tokenizer_has_next_char(tokenizer, '/')) {
					tokenizer->read_position += 2; // consume */
					return true;
				}
			} else {
				tokenizer->read_position += 1;
			}
		}
	} else {
		unreachable();
	}

	return false;
}

Token _tokenizer_create_eof_token(Tokenizer* tokenizer) {
	return (Token) {
		.source_range = (SourceRange) {
			.start = tokenizer->read_position,
			.end = tokenizer->read_position,
		},
		.string = (String) {},
		.kind = TOKEN_EOF,
	};
}

Token tokenizer_next_token(Tokenizer* tokenizer) {
	char32_t current_char = 0;
	while (true) {
		if (tokenizer_is_end(tokenizer)) {
			return _tokenizer_create_eof_token(tokenizer);
		}

		while (true) {
			if (tokenizer_is_end(tokenizer)) {
				return _tokenizer_create_eof_token(tokenizer);
			}

			bool skipped = _tokenizer_try_skip_comment(tokenizer);
			if (!skipped) {
				break;
			}
		}

		current_char = tokenizer_get_char(tokenizer);
		if (!isspace(current_char)) {
			break;
		} else {
			tokenizer->read_position += 1;
		}
	}

	switch (current_char) {
	case '#':
		return _tokenizer_create_single_char_token(tokenizer, TOKEN_HASH);
	case '*':
		return _tokenizer_try_create_double_char_token(tokenizer, '*', '=', TOKEN_ASTERISK, TOKEN_ASSIGNMENT_BY_PRODUCT);
	case '%':
		return _tokenizer_try_create_double_char_token(tokenizer, '%', '=', TOKEN_PERCENT, TOKEN_ASSIGNMENT_BY_REMAINDER);
	case ',':
		return _tokenizer_create_single_char_token(tokenizer, TOKEN_COMMA);
	case '.':
		return _tokenizer_create_single_char_token(tokenizer, TOKEN_DOT);
	case '=':
		return _tokenizer_try_create_double_char_token(tokenizer, '=', '=', TOKEN_EQUAL, TOKEN_DOUBLE_EQUAL);
	case ':':
		return _tokenizer_create_single_char_token(tokenizer, TOKEN_COLON);
	case ';':
		return _tokenizer_create_single_char_token(tokenizer, TOKEN_SEMICOLON);
	case '&': {
		if (tokenizer->read_position + 1 < tokenizer->source_code.length) {
			TokenKind kind = TOKEN_COUNT;
			switch (tokenizer->source_code.v[tokenizer->read_position + 1]) {
			case '&':
				kind = TOKEN_LOGIC_AND;
				break;
			case '=':
				kind = TOKEN_ASSIGNMENT_BY_BITWISE_AND;
				break;
			default:
				break;
			}

			if (kind != TOKEN_COUNT) {
				Token token = (Token) {
					.source_range = (SourceRange) {
						.start = tokenizer->read_position,
						.end = tokenizer->read_position + 2,
					},
					.string = sub_str(tokenizer->source_code, tokenizer->read_position, 2),
					.kind = kind,
				};

				tokenizer->read_position += 2;
				return token;
			}
		}

		return _tokenizer_create_single_char_token(tokenizer, TOKEN_AMPERSAND);
	}
	case '|': {
		if (tokenizer->read_position + 1 < tokenizer->source_code.length) {
			TokenKind kind = TOKEN_COUNT;
			switch (tokenizer->source_code.v[tokenizer->read_position + 1]) {
			case '|':
				kind = TOKEN_LOGIC_OR;
				break;
			case '=':
				kind = TOKEN_ASSIGNMENT_BY_BITWISE_OR;
				break;
			default:
				break;
			}

			if (kind != TOKEN_COUNT) {
				Token token = (Token) {
					.source_range = (SourceRange) {
						.start = tokenizer->read_position,
						.end = tokenizer->read_position + 2,
					},
					.string = sub_str(tokenizer->source_code, tokenizer->read_position, 2),
					.kind = kind,
				};

				tokenizer->read_position += 2;
				return token;
			}
		}

		return _tokenizer_create_single_char_token(tokenizer, TOKEN_PIPE);
	}
	case '^':
		return _tokenizer_try_create_double_char_token(tokenizer, '^', '=', TOKEN_BITWISE_XOR, TOKEN_ASSIGNMENT_BY_BITWISE_XOR);
	case '~':
		return _tokenizer_create_single_char_token(tokenizer, TOKEN_BITWISE_NOT);
	case '!':
		return _tokenizer_try_create_double_char_token(tokenizer, '!', '=', TOKEN_EXCLAMATION_MARK, TOKEN_NOT_EQUAL);
	case '?':
		return _tokenizer_create_single_char_token(tokenizer, TOKEN_QUESTION_MARK);
	case '/':
		return _tokenizer_try_create_double_char_token(tokenizer, '/', '=', TOKEN_FORWARD_SLASH, TOKEN_ASSIGNMENT_BY_QUOTIENT);
	case '+': {
		if (tokenizer->read_position + 1 < tokenizer->source_code.length) {
			TokenKind kind = TOKEN_COUNT;
			switch (tokenizer->source_code.v[tokenizer->read_position + 1]) {
			case '+':
				kind = TOKEN_DOUBLE_PLUS;
				break;
			case '=':
				kind = TOKEN_ASSIGNMENT_BY_SUM;
				break;
			default:
				break;
			}

			if (kind != TOKEN_COUNT) {
				Token token = (Token) {
					.source_range = (SourceRange) {
						.start = tokenizer->read_position,
						.end = tokenizer->read_position + 2,
					},
					.string = sub_str(tokenizer->source_code, tokenizer->read_position, 2),
					.kind = kind,
				};

				tokenizer->read_position += 2;
				return token;
			}
		}

		return _tokenizer_create_single_char_token(tokenizer, TOKEN_PLUS);
	}
	case '-': {
		if (tokenizer->read_position + 1 < tokenizer->source_code.length) {
			TokenKind kind = TOKEN_COUNT;
			switch (tokenizer->source_code.v[tokenizer->read_position + 1]) {
			case '-':
				kind = TOKEN_DOUBLE_MINUS;
				break;
			case '=':
				kind = TOKEN_ASSIGNMENT_BY_DIFFERENCE;
				break;
			case '>':
				kind = TOKEN_ARROW;
				break;
			default:
				break;
			}

			if (kind != TOKEN_COUNT) {
				Token token = (Token) {
					.source_range = (SourceRange) {
						.start = tokenizer->read_position,
						.end = tokenizer->read_position + 2,
					},
					.string = sub_str(tokenizer->source_code, tokenizer->read_position, 2),
					.kind = kind,
				};

				tokenizer->read_position += 2;
				return token;
			}
		}

		return _tokenizer_create_single_char_token(tokenizer, TOKEN_MINUS);
	}

	case '(':
		return _tokenizer_create_single_char_token(tokenizer, TOKEN_LEFT_PAREN);
	case ')':
		return _tokenizer_create_single_char_token(tokenizer, TOKEN_RIGHT_PAREN);

	case '[':
		return _tokenizer_create_single_char_token(tokenizer, TOKEN_LEFT_BRACKET);
	case ']':
		return _tokenizer_create_single_char_token(tokenizer, TOKEN_RIGHT_BRACKET);

	case '{':
		return _tokenizer_create_single_char_token(tokenizer, TOKEN_LEFT_BRACE);
	case '}':
		return _tokenizer_create_single_char_token(tokenizer, TOKEN_RIGHT_BRACE);

	case '>':
		return _tokenizer_try_create_double_char_token(tokenizer, '>', '=', TOKEN_GREATER, TOKEN_GREATER_OR_EQUAL);
	case '<':
		return _tokenizer_try_create_double_char_token(tokenizer, '<', '=', TOKEN_LESS, TOKEN_LESS_OR_EQUAL);
	case '"': {
		Token string_token = {};
		StringTokenizerResult result = _tokenizer_try_create_string_token(tokenizer, '"', '"', &string_token);
		assert(result == STR_TOKEN_RESULT_NONE);
		return string_token;
	}
	case '\'': {
		Token string_token = {};
		StringTokenizerResult result = _tokenizer_try_create_string_token(tokenizer, '\'', '\'', &string_token);
		assert(result == STR_TOKEN_RESULT_NONE);
		return string_token;
	}
	}

	Token ident_token = {};
	if (_tokenizer_try_create_ident_token(tokenizer, &ident_token)) {
		return ident_token;
	}

	unreachable_msg("Unhandled char during creation of the next token");
	return (Token) {};
}
