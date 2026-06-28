#ifndef TOKENIZER_H
#define TOKENIZER_H

#include "core/core.h"
#include "parser/source_info.h"

typedef enum {
	TOKEN_EOF,

	TOKEN_IDENT,
	TOKEN_STRING,
	TOKEN_CHAR,

	TOKEN_HASH,
	TOKEN_DOUBLE_HASH,
	TOKEN_COMMA,
	TOKEN_DOT,
	TOKEN_COLON,
	TOKEN_SEMICOLON,
	TOKEN_AMPERSAND,
	TOKEN_PIPE,
	TOKEN_EXCLAMATION_MARK,
	TOKEN_QUESTION_MARK,
	TOKEN_FORWARD_SLASH,
	TOKEN_BACKWARD_SLASH,
	TOKEN_ARROW,
	TOKEN_ELLIPSES,

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
	TOKEN_PERCENT,

	TOKEN_DOUBLE_PLUS,
	TOKEN_DOUBLE_MINUS,

	TOKEN_ASSIGNMENT_BY_SUM,
	TOKEN_ASSIGNMENT_BY_DIFFERENCE,
	TOKEN_ASSIGNMENT_BY_PRODUCT,
	TOKEN_ASSIGNMENT_BY_QUOTIENT,
	TOKEN_ASSIGNMENT_BY_REMAINDER,

	TOKEN_ASSIGNMENT_BY_BITWISE_AND,
	TOKEN_ASSIGNMENT_BY_BITWISE_OR,
	TOKEN_ASSIGNMENT_BY_BITWISE_XOR,

	TOKEN_ASSIGNMENT_BY_BITWISE_SHIFT_LEFT,
	TOKEN_ASSIGNMENT_BY_BITWISE_SHIFT_RIGHT,

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

	// Bitwise
	TOKEN_BITWISE_XOR,
	TOKEN_BITWISE_NOT,
	TOKEN_BITWISE_SHIFT_LEFT,
	TOKEN_BITWISE_SHIFT_RIGHT,

	// Keywords
	TOKEN_KEYWORD_TYPEDEF,
	TOKEN_KEYWORD_STRUCT,
	TOKEN_KEYWORD_UNION,
	TOKEN_KEYWORD_ENUM,
	TOKEN_KEYWORD_CONST,
	TOKEN_KEYWORD_RETURN,
	TOKEN_KEYWORD_INLINE,
	TOKEN_KEYWORD_EXTERN,
	TOKEN_KEYWORD_STATIC,
	TOKEN_KEYWORD_IF,
	TOKEN_KEYWORD_ELSE,

	TOKEN_KEYWORD_VOID,
	TOKEN_KEYWORD_SIZE_T,

	TOKEN_KEYWORD_FLOAT,
	TOKEN_KEYWORD_DOUBLE,

	TOKEN_KEYWORD_CHAR,
	TOKEN_KEYWORD_SHORT,
	TOKEN_KEYWORD_INT,
	TOKEN_KEYWORD_LONG,
	TOKEN_KEYWORD_SIGNED,
	TOKEN_KEYWORD_UNSIGNED,

	TOKEN_KEYWORD_INT8,
	TOKEN_KEYWORD_INT16,
	TOKEN_KEYWORD_INT32,
	TOKEN_KEYWORD_INT64,

	TOKEN_DECLSPEC,

	TOKEN_COUNT,
} TokenKind;

String token_kind_to_string(TokenKind kind);

typedef struct {
	const SourceFile* source_file;
	String source_code;
	size_t read_position;
} Tokenizer;

typedef struct {
	String string;
	TokenKind kind;
	SourceRange source_range;
} Token;

typedef struct {
	Token* tokens;
	size_t count;
} TokenArray;

typedef enum {
	STR_TOKEN_RESULT_NONE,
	STR_TOKEN_RESULT_NO_CLOSING_CHAR,
	STR_TOKEN_RESULT_NEWLINE_REACHED,
	STR_TOKEN_RESULT_INVALID_ESCAPE_CHAR,
	STR_TOKEN_RESULT_EOF_REACHED,
} StringTokenizerResult;

inline SourceString source_string_from_token(Token token) {
	return (SourceString) {
		.source_file = token.source_range.source_file,
		.string = token.string,
	};
}

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

void tokenizer_init(Tokenizer* tokenizer, const SourceFile* source_file);
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

inline void tokenizer_reset_to_token(Tokenizer* tokenizer, Token token) {
	tokenizer->read_position = token.source_range.end;
}

#endif
