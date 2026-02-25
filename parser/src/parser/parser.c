#include "parser.h"

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
		if (current_char >= 'a' && current_char <= 'z'
				|| current_char >= 'A' && current_char <= 'Z'
				|| current_char >= '0' && current_char <= '9'
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

Token tokenizer_next_token(Tokenizer* tokenizer) {
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

	char32_t current_char = tokenizer_get_char(tokenizer);
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
	}

	Token ident_token = {};
	if (_tokenizer_try_create_ident_token(tokenizer, &ident_token)) {
		return ident_token;
	}

	unreachable_msg("Unhandled char during creation of the next token");
	return (Token) {};
}
