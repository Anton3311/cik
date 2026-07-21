#include "tokenizer.h"

#include <ctype.h>

void tokenizer_init(Tokenizer* tokenizer, const SourceFile* source_file) {
	assert(source_file);

	tokenizer->source_file = source_file;
	tokenizer->source_code = source_file->source_code;
	tokenizer->read_position = 0;
}

inline char32_t _tokenizer_get_char(const Tokenizer* tokenizer) {
	assert(!tokenizer_is_end(tokenizer));
	return (char32_t)tokenizer->source_code.v[tokenizer->read_position];
}

inline bool _tokenizer_has_next_char(Tokenizer* tokenizer, char32_t next_char) {
	if (tokenizer_is_end(tokenizer)) {
		return false;
	}

	return tokenizer->source_code.v[tokenizer->read_position + 1] == next_char;
}

String token_kind_to_string(TokenKind kind) {
	switch (kind) {
	case TOKEN_EOF: return STR_LIT("<eof>");

	case TOKEN_IDENT: return STR_LIT("<identifier>");
	case TOKEN_STRING: return STR_LIT("<string>");
	case TOKEN_CHAR: return STR_LIT("<char>");

	case TOKEN_HASH: return STR_LIT("#");
	case TOKEN_DOUBLE_HASH: return STR_LIT("##");
	case TOKEN_COMMA: return STR_LIT(",");
	case TOKEN_DOT: return STR_LIT(".");
	case TOKEN_COLON: return STR_LIT(":");
	case TOKEN_SEMICOLON: return STR_LIT(";");
	case TOKEN_AMPERSAND: return STR_LIT("&");
	case TOKEN_PIPE: return STR_LIT("|");
	case TOKEN_EXCLAMATION_MARK: return STR_LIT("!");
	case TOKEN_QUESTION_MARK: return STR_LIT("?");
	case TOKEN_FORWARD_SLASH: return STR_LIT("/");
	case TOKEN_BACKWARD_SLASH: return STR_LIT("\\");
	case TOKEN_ARROW: return STR_LIT("->");
	case TOKEN_ELLIPSES: return STR_LIT("...");

	// Parens & friends
	case TOKEN_LEFT_PAREN: return STR_LIT("(");
	case TOKEN_RIGHT_PAREN: return STR_LIT(")");

	case TOKEN_LEFT_BRACE: return STR_LIT("{");
	case TOKEN_RIGHT_BRACE: return STR_LIT("}");

	case TOKEN_LEFT_BRACKET: return STR_LIT("[");
	case TOKEN_RIGHT_BRACKET: return STR_LIT("]");

	// Arithmetics
	case TOKEN_PLUS: return STR_LIT("+");
	case TOKEN_MINUS: return STR_LIT("-");
	case TOKEN_ASTERISK: return STR_LIT("*");
	case TOKEN_PERCENT: return STR_LIT("%");

	case TOKEN_DOUBLE_PLUS: return STR_LIT("++");
	case TOKEN_DOUBLE_MINUS: return STR_LIT("--");

	case TOKEN_ASSIGNMENT_BY_SUM: return STR_LIT("+=");
	case TOKEN_ASSIGNMENT_BY_DIFFERENCE: return STR_LIT("-=");
	case TOKEN_ASSIGNMENT_BY_PRODUCT: return STR_LIT("*=");
	case TOKEN_ASSIGNMENT_BY_QUOTIENT: return STR_LIT("/=");
	case TOKEN_ASSIGNMENT_BY_REMAINDER: return STR_LIT("%=");

	case TOKEN_ASSIGNMENT_BY_BITWISE_AND: return STR_LIT("&=");
	case TOKEN_ASSIGNMENT_BY_BITWISE_OR: return STR_LIT("|=");
	case TOKEN_ASSIGNMENT_BY_BITWISE_XOR: return STR_LIT("^=");
	case TOKEN_ASSIGNMENT_BY_BITWISE_SHIFT_LEFT: return STR_LIT("<<=");
	case TOKEN_ASSIGNMENT_BY_BITWISE_SHIFT_RIGHT: return STR_LIT(">>=");

	// Comparison
	case TOKEN_LESS: return STR_LIT("<");
	case TOKEN_GREATER: return STR_LIT(">");
	case TOKEN_LESS_OR_EQUAL: return STR_LIT("<=");
	case TOKEN_GREATER_OR_EQUAL: return STR_LIT(">=");
	case TOKEN_EQUAL: return STR_LIT("=");
	case TOKEN_NOT_EQUAL: return STR_LIT("!=");
	case TOKEN_DOUBLE_EQUAL: return STR_LIT("==");

	// Logic
	case TOKEN_LOGIC_AND: return STR_LIT("&&");
	case TOKEN_LOGIC_OR: return STR_LIT("||");

	// Bitwise
	case TOKEN_BITWISE_XOR: return STR_LIT("^");
	case TOKEN_BITWISE_NOT: return STR_LIT("~");
	case TOKEN_BITWISE_SHIFT_LEFT: return STR_LIT("<<");
	case TOKEN_BITWISE_SHIFT_RIGHT: return STR_LIT(">>");

	// Keywords
	case TOKEN_KEYWORD_TYPEDEF: return STR_LIT("typedef");
	case TOKEN_KEYWORD_STRUCT: return STR_LIT("struct");
	case TOKEN_KEYWORD_UNION: return STR_LIT("union");
	case TOKEN_KEYWORD_ENUM: return STR_LIT("enum");
	case TOKEN_KEYWORD_CONST: return STR_LIT("const");
	case TOKEN_KEYWORD_RETURN: return STR_LIT("return");
	case TOKEN_KEYWORD_INLINE: return STR_LIT("inline");
	case TOKEN_KEYWORD_EXTERN: return STR_LIT("extern");
	case TOKEN_KEYWORD_STATIC: return STR_LIT("static");
	case TOKEN_KEYWORD_IF: return STR_LIT("if");
	case TOKEN_KEYWORD_ELSE: return STR_LIT("else");
	case TOKEN_KEYWORD_WHILE: return STR_LIT("while");
	case TOKEN_KEYWORD_DO: return STR_LIT("do");
	case TOKEN_KEYWORD_FOR: return STR_LIT("for");
	case TOKEN_KEYWORD_BREAK: return STR_LIT("break");
	case TOKEN_KEYWORD_CONTINUE: return STR_LIT("continue");

	case TOKEN_KEYWORD_VOID: return STR_LIT("void");
	case TOKEN_KEYWORD_SIZE_T: return STR_LIT("size_t");

	case TOKEN_KEYWORD_FLOAT: return STR_LIT("float");
	case TOKEN_KEYWORD_DOUBLE: return STR_LIT("double");

	case TOKEN_KEYWORD_CHAR: return STR_LIT("char");
	case TOKEN_KEYWORD_SHORT: return STR_LIT("short");
	case TOKEN_KEYWORD_INT: return STR_LIT("int");
	case TOKEN_KEYWORD_LONG: return STR_LIT("long");
	case TOKEN_KEYWORD_SIGNED: return STR_LIT("signed");
	case TOKEN_KEYWORD_UNSIGNED: return STR_LIT("unsigned");

	case TOKEN_KEYWORD_INT8: return STR_LIT("__int8");
	case TOKEN_KEYWORD_INT16: return STR_LIT("__int16");
	case TOKEN_KEYWORD_INT32: return STR_LIT("__int32");
	case TOKEN_KEYWORD_INT64: return STR_LIT("__int64");

	case TOKEN_DECLSPEC: return STR_LIT("__declspec");
	}

	unreachable();
	return (String) {};
}

bool _tokenizer_try_skip_comment(Tokenizer* tokenizer);

inline Token _tokenizer_create_single_char_token(Tokenizer* tokenizer, TokenKind kind) {
	size_t position = tokenizer->read_position;
	Token token = (Token) {
		.source_range = (SourceRange) {
			.source_file = tokenizer->source_file,
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
	profile_scope_start(__func__);
	size_t token_start = tokenizer->read_position;

	while (true) {
		if (tokenizer_is_end(tokenizer)) {
			break;
		}

		char32_t current_char = _tokenizer_get_char(tokenizer);
		if ((current_char >= 'a' && current_char <= 'z')
				|| (current_char >= 'A' && current_char <= 'Z')
				|| (current_char >= '0' && current_char <= '9')
				|| current_char == '_'
				|| current_char == '$'
				|| current_char == '@') {
			tokenizer->read_position += 1;
		} else {
			break;
		}
	}

	size_t string_length = tokenizer->read_position - token_start;
	if (string_length == 0) {
		profile_scope_end();
		return false;
	}

	String token_string = sub_str(tokenizer->source_code, token_start, string_length);
	TokenKind token_kind = TOKEN_IDENT;
	if (str_equal(token_string, STR_LIT("typedef"))) {
		token_kind = TOKEN_KEYWORD_TYPEDEF;
	} else if (str_equal(token_string, STR_LIT("struct"))) {
		token_kind = TOKEN_KEYWORD_STRUCT;
	} else if (str_equal(token_string, STR_LIT("union"))) {
		token_kind = TOKEN_KEYWORD_UNION;
	} else if (str_equal(token_string, STR_LIT("enum"))) {
		token_kind = TOKEN_KEYWORD_ENUM;
	} else if (str_equal(token_string, STR_LIT("const"))) {
		token_kind = TOKEN_KEYWORD_CONST;
	} else if (str_equal(token_string, STR_LIT("return"))) {
		token_kind = TOKEN_KEYWORD_RETURN;
	} else if (str_equal(token_string, STR_LIT("inline"))) {
		token_kind = TOKEN_KEYWORD_INLINE;
	} else if (str_equal(token_string, STR_LIT("__inline"))) {
		token_kind = TOKEN_KEYWORD_INLINE;
	} else if (str_equal(token_string, STR_LIT("extern"))) {
		token_kind = TOKEN_KEYWORD_EXTERN;
	} else if (str_equal(token_string, STR_LIT("static"))) {
		token_kind = TOKEN_KEYWORD_STATIC;
	} else if (str_equal(token_string, STR_LIT("__declspec"))) {
		token_kind = TOKEN_DECLSPEC;
	} else if (str_equal(token_string, STR_LIT("if"))) {
		token_kind = TOKEN_KEYWORD_IF;
	} else if (str_equal(token_string, STR_LIT("else"))) {
		token_kind = TOKEN_KEYWORD_ELSE;
	} else if (str_equal(token_string, STR_LIT("while"))) {
		token_kind = TOKEN_KEYWORD_WHILE;
	} else if (str_equal(token_string, STR_LIT("do"))) {
		token_kind = TOKEN_KEYWORD_DO;
	} else if (str_equal(token_string, STR_LIT("for"))) {
		token_kind = TOKEN_KEYWORD_FOR;
	} else if (str_equal(token_string, STR_LIT("break"))) {
		token_kind = TOKEN_KEYWORD_BREAK;
	} else if (str_equal(token_string, STR_LIT("continue"))) {
		token_kind = TOKEN_KEYWORD_CONTINUE;
	} else if (str_equal(token_string, STR_LIT("void"))) {
		token_kind = TOKEN_KEYWORD_VOID;
	} else if (str_equal(token_string, STR_LIT("size_t"))) {
		token_kind = TOKEN_KEYWORD_SIZE_T;
	} else if (str_equal(token_string, STR_LIT("float"))) {
		token_kind = TOKEN_KEYWORD_FLOAT;
	} else if (str_equal(token_string, STR_LIT("double"))) {
		token_kind = TOKEN_KEYWORD_DOUBLE;
	} else if (str_equal(token_string, STR_LIT("char"))) {
		token_kind = TOKEN_KEYWORD_CHAR;
	} else if (str_equal(token_string, STR_LIT("short"))) {
		token_kind = TOKEN_KEYWORD_SHORT;
	} else if (str_equal(token_string, STR_LIT("int"))) {
		token_kind = TOKEN_KEYWORD_INT;
	} else if (str_equal(token_string, STR_LIT("long"))) {
		token_kind = TOKEN_KEYWORD_LONG;
	} else if (str_equal(token_string, STR_LIT("signed"))) {
		token_kind = TOKEN_KEYWORD_SIGNED;
	} else if (str_equal(token_string, STR_LIT("unsigned"))) {
		token_kind = TOKEN_KEYWORD_UNSIGNED;
	} else if (str_equal(token_string, STR_LIT("__int8"))) {
		token_kind = TOKEN_KEYWORD_INT8;
	} else if (str_equal(token_string, STR_LIT("__int16"))) {
		token_kind = TOKEN_KEYWORD_INT16;
	} else if (str_equal(token_string, STR_LIT("__int32"))) {
		token_kind = TOKEN_KEYWORD_INT32;
	} else if (str_equal(token_string, STR_LIT("__int64"))) {
		token_kind = TOKEN_KEYWORD_INT64;
	}

	*out_token = (Token) {
		.source_range = (SourceRange) {
			.source_file = tokenizer->source_file,
			.start = token_start,
			.end = tokenizer->read_position,
		},
		.string = token_string,
		.kind = token_kind,
	};

	profile_scope_end();
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

		char32_t current_char = _tokenizer_get_char(tokenizer);
		if (!isspace(current_char)) {
			break;
		} else {
			tokenizer->read_position += 1;
		}
	}
}

// NOTE: Returnd token includes quotation marks
StringTokenizerResult tokenizer_try_create_string_token(Tokenizer* tokenizer,
		char32_t string_opening_char,
		char32_t string_closing_char,
		Token* out_token) {
	size_t string_start = tokenizer->read_position;
	char32_t opening_char = _tokenizer_get_char(tokenizer);

	// NOTE: This shouldn't be triggerable by the user code,
	//       only by missuse of this function.
	assert(opening_char == string_opening_char);
	tokenizer->read_position += 1;

	while (true) {
		if (tokenizer_is_end(tokenizer)) {
			return STR_TOKEN_RESULT_EOF_REACHED;
		}

		char32_t current_char = _tokenizer_get_char(tokenizer);
		if (current_char == '\\') {
			tokenizer->read_position += 1;
			if (tokenizer_is_end(tokenizer)) {
				return STR_TOKEN_RESULT_EOF_REACHED;
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
					.source_file = tokenizer->source_file,
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
	char32_t c = _tokenizer_get_char(tokenizer);
	tokenizer->read_position += 1;
	assert(c == first_char);

	if (!tokenizer_is_end(tokenizer)) {
		char32_t c2 = _tokenizer_get_char(tokenizer);
		if (c2 == second_char) {
			tokenizer->read_position += 1;
			return (Token) {
				.source_range = (SourceRange) {
					.source_file = tokenizer->source_file,
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
			.source_file = tokenizer->source_file,
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
	char32_t current_char = _tokenizer_get_char(tokenizer);
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
			char32_t c = _tokenizer_get_char(tokenizer);
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

			char32_t c = _tokenizer_get_char(tokenizer);
			if (c == '*' && _tokenizer_has_next_char(tokenizer, '/')) {
				tokenizer->read_position += 2; // consume */
				return true;
			}

			tokenizer->read_position += 1;
		}
	} else {
		unreachable();
	}

	return false;
}

Token _tokenizer_create_eof_token(Tokenizer* tokenizer) {
	return (Token) {
		.source_range = (SourceRange) {
			.source_file = tokenizer->source_file,
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

		current_char = _tokenizer_get_char(tokenizer);
		if (!isspace(current_char)) {
			break;
		} else {
			tokenizer->read_position += 1;
		}
	}

	switch (current_char) {
	case '#':
		return _tokenizer_try_create_double_char_token(tokenizer, '#', '#', TOKEN_HASH, TOKEN_DOUBLE_HASH);
	case '*':
		return _tokenizer_try_create_double_char_token(tokenizer, '*', '=', TOKEN_ASTERISK, TOKEN_ASSIGNMENT_BY_PRODUCT);
	case '%':
		return _tokenizer_try_create_double_char_token(tokenizer, '%', '=', TOKEN_PERCENT, TOKEN_ASSIGNMENT_BY_REMAINDER);
	case ',':
		return _tokenizer_create_single_char_token(tokenizer, TOKEN_COMMA);
	case '.': {
		if (tokenizer->read_position + 3 <= tokenizer->source_code.length) {
			const char* string = tokenizer->source_code.v + tokenizer->read_position;
			bool is_ellipses = string[0] == '.' && string[1] == '.' && string[2] == '.';
			if (is_ellipses) {
				Token token = (Token) {
					.source_range = (SourceRange) {
						.source_file = tokenizer->source_file,
						.start = tokenizer->read_position,
						.end = tokenizer->read_position + 3,
					},
					.string = sub_str(tokenizer->source_code, tokenizer->read_position, 3),
					.kind = TOKEN_ELLIPSES,
				};

				tokenizer->read_position += 3;
				return token;
			}
		}

		return _tokenizer_create_single_char_token(tokenizer, TOKEN_DOT);
	}
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
						.source_file = tokenizer->source_file,
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
						.source_file = tokenizer->source_file,
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
	case '\\':
		return _tokenizer_create_single_char_token(tokenizer, TOKEN_BACKWARD_SLASH);
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
						.source_file = tokenizer->source_file,
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
						.source_file = tokenizer->source_file,
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
		if (tokenizer->read_position + 1 < tokenizer->source_code.length) {
			size_t token_length = 1;
			TokenKind kind = TOKEN_COUNT;
			switch (tokenizer->source_code.v[tokenizer->read_position + 1]) {
			case '>':
				if (tokenizer->read_position + 2 < tokenizer->source_code.length
						&& tokenizer->source_code.v[tokenizer->read_position + 2] == '=') {
					kind = TOKEN_ASSIGNMENT_BY_BITWISE_SHIFT_RIGHT;
					token_length = 3;
				} else {
					kind = TOKEN_BITWISE_SHIFT_RIGHT;
					token_length = 2;
				}
				break;
			case '=':
				kind = TOKEN_GREATER_OR_EQUAL;
				token_length = 2;
				break;
			default:
				break;
			}

			if (kind != TOKEN_COUNT) {
				Token token = (Token) {
					.source_range = (SourceRange) {
						.source_file = tokenizer->source_file,
						.start = tokenizer->read_position,
						.end = tokenizer->read_position + token_length,
					},
					.string = sub_str(tokenizer->source_code, tokenizer->read_position, token_length),
					.kind = kind,
				};

				tokenizer->read_position += token_length;
				return token;
			}
		}

		return _tokenizer_create_single_char_token(tokenizer, TOKEN_GREATER);
	case '<':
		if (tokenizer->read_position + 1 < tokenizer->source_code.length) {
			size_t token_length = 1;
			TokenKind kind = TOKEN_COUNT;
			switch (tokenizer->source_code.v[tokenizer->read_position + 1]) {
			case '<':
				if (tokenizer->read_position + 2 < tokenizer->source_code.length
						&& tokenizer->source_code.v[tokenizer->read_position + 2] == '=') {
					kind = TOKEN_ASSIGNMENT_BY_BITWISE_SHIFT_LEFT;
					token_length = 3;
				} else {
					kind = TOKEN_BITWISE_SHIFT_LEFT;
					token_length = 2;
				}
				break;
			case '=':
				kind = TOKEN_LESS_OR_EQUAL;
				token_length = 2;
				break;
			default:
				break;
			}

			if (kind != TOKEN_COUNT) {
				Token token = (Token) {
					.source_range = (SourceRange) {
						.source_file = tokenizer->source_file,
						.start = tokenizer->read_position,
						.end = tokenizer->read_position + token_length,
					},
					.string = sub_str(tokenizer->source_code, tokenizer->read_position, token_length),
					.kind = kind,
				};

				tokenizer->read_position += token_length;
				return token;
			}
		}

		return _tokenizer_create_single_char_token(tokenizer, TOKEN_LESS);
	case '"': {
		Token string_token = {};
		StringTokenizerResult result = tokenizer_try_create_string_token(tokenizer, '"', '"', &string_token);
		assert(result == STR_TOKEN_RESULT_NONE);
		return string_token;
	}
	case '\'': {
		Token char_token = {};
		StringTokenizerResult result = tokenizer_try_create_string_token(tokenizer, '\'', '\'', &char_token);
		assert(result == STR_TOKEN_RESULT_NONE);
		char_token.kind = TOKEN_CHAR;
		return char_token;
	}
	}

	Token ident_token = {};
	if (_tokenizer_try_create_ident_token(tokenizer, &ident_token)) {
		return ident_token;
	}

	unreachable_msg("Unhandled char during creation of the next token");
	return (Token) {};
}
