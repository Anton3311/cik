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
	TOKEN_STRING,

	TOKEN_HASH,
	TOKEN_COMMA,
	TOKEN_DOT,
	TOKEN_COLON,
	TOKEN_SEMICOLON,
	TOKEN_AMPERSAND,
	TOKEN_PIPE,
	TOKEN_EXCLAMATION_MARK,
	TOKEN_ARROW,

	// Parens & friends
	TOKEN_LEFT_PAREN,
	TOKEN_RIGHT_PAREN,

	TOKEN_LEFT_BRACE,
	TOKEN_RIGHT_BRACE,

	TOKEN_LEFT_BRACKET,
	TOKEN_RIGHT_BRACKET,

	// Arithmetics
	TOKEN_PLUS,
	TOKEN_MINUS,
	TOKEN_ASTERISK,

	// Comparison
	TOKEN_LESS,
	TOKEN_GREATER,
	TOKEN_LESS_OR_EQUAL,
	TOKEN_GREATER_OR_EQUAL,
	TOKEN_EQUAL,
	TOKEN_NOT_EQUAL,
	TOKEN_DOUBLE_EQUAL,

	// Logic
	TOKEN_LOGIC_AND,
	TOKEN_LOGIC_OR,
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

typedef enum {
	STR_TOKEN_RESULT_NONE,
	STR_TOKEN_RESULT_NO_CLOSING_CHAR,
	STR_TOKEN_RESULT_NEWLINE_REACHED,
	STR_TOKEN_RESULT_INVALID_ESCAPE_CHAR,
	STR_TOKEN_RESULT_EOF_REACHED,
} StringTokenizerResult;

inline bool tokenizer_is_end(const Tokenizer* tokenizer) {
	return tokenizer->read_position == tokenizer->source_code.length;
}

inline char32_t tokenizer_get_char(const Tokenizer* tokenizer) {
	assert(!tokenizer_is_end(tokenizer));
	return (char32_t)tokenizer->source_code.v[tokenizer->read_position];
}

inline bool _tokenizer_has_next_char(Tokenizer* tokenizer, char32_t next_char) {
	if (tokenizer_is_end(tokenizer)) {
		return false;
	}

	return tokenizer->source_code.v[tokenizer->read_position + 1] == next_char;
}

void tokenizer_skip_whitespace_and_comments(Tokenizer* tokenizer);

StringTokenizerResult _tokenizer_try_create_string_token(Tokenizer* tokenizer,
		char32_t string_opening_char,
		char32_t string_closing_char,
		Token* out_token);

Token tokenizer_next_token(Tokenizer* tokenizer);

inline Token tokenizer_view_next(Tokenizer* tokenizer) {
	Tokenizer saved_tokenizer = *tokenizer;
	Token next_token = tokenizer_next_token(tokenizer);
	*tokenizer = saved_tokenizer;
	return next_token;
}

#endif
