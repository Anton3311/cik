#include "parser.h"

#include <ctype.h>

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

	*out_token = (Token) {
		.source_range = (SourceRange) {
			.start = token_start,
			.end = tokenizer->read_position,
		},
		.string = sub_str(tokenizer->source_code, token_start, string_length),
		.kind = TOKEN_IDENT,
	};
	return true;
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

Token tokenizer_next_token(Tokenizer* tokenizer) {
	char32_t current_char = 0;
	while (true) {
		if (tokenizer_is_end(tokenizer)) {
			return (Token) {
				.source_range = (SourceRange) {
					.start = tokenizer->read_position,
						.end = tokenizer->read_position,
				},
					.string = (String) {},
					.kind = TOKEN_EOF,
			};
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

	case '<': {
		Token string_token = {};
		StringTokenizerResult result = _tokenizer_try_create_string_token(tokenizer, '<', '>', &string_token);
		assert(result == STR_TOKEN_RESULT_NONE);
		return string_token;
	}
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
