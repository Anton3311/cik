#ifndef PARSER_H
#define PARSER_H

#include "core/core.h"

typedef struct {
	size_t start;
	size_t end;
} SourceRange;

typedef enum {
	TOKEN_EOF,

	TOKEN_IDENT,

	TOKEN_HASH,

	// Parens & friends
	TOKEN_LEFT_PAREN,
	TOKEN_RIGHT_PAREN,

	TOKEN_LEFT_BRACE,
	TOKEN_RIGHT_BRACE,

	TOKEN_LEFT_BRACKET,
	TOKEN_RIGHT_BRACKET,

	// Comparison
	TOKEN_LESS,
	TOKEN_GREATER,
	TOKEN_EQUAL,
	TOKEN_DOUBLE_EQUAL,
} TokenKind;

typedef struct {
	String source_code;
	size_t read_position;
} Tokenizer;

typedef struct {
	SourceRange source_range;
	String string;
	TokenKind kind;
} Token;

inline bool tokenizer_is_end(const Tokenizer* tokenizer) {
	return tokenizer->read_position == tokenizer->source_code.length;
}

inline char32_t tokenizer_get_char(const Tokenizer* tokenizer) {
	assert(!tokenizer_is_end(tokenizer));
	return (char32_t)tokenizer->source_code.v[tokenizer->read_position];
}

Token tokenizer_next_token(Tokenizer* tokenizer);

#endif
