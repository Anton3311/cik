#ifndef PREPROCESSOR_H
#define PREPROCESSOR_H

#include "parser/tokenizer.h"

//
// LineInfo
//

typedef struct {
	uint32_t line;
	uint32_t column;
} SourceLocation;

typedef struct {
	uint32_t line_count;
	uint32_t* line_starts;
} LineInfo;

LineInfo line_info_from_source(Arena* allocator, String source);
SourceLocation line_info_pos_to_source_location(const LineInfo* line_info, size_t string_pos);
String line_info_get_line_string(const LineInfo* line_info, String source_code, size_t line_index);

//
// Preprocessor
//

typedef struct {
	Tokenizer tokenizer;
	LineInfo line_info;
} Preprocessor;

typedef enum {
	MACRO_STYLE_DEFAULT,
	MACRO_STYLE_FUNCTION,
} MacroStyle;

typedef struct {
	String name;

	String* parameter_names;
	size_t parameter_count;
	
	Token* tokens;
	size_t token_count;
} MacroDefinition;

void preprocessor_init(Preprocessor* state, String source_code, Arena* allocator);
void preprocessor_skip_derective(Preprocessor* state);

inline Token preprocessor_next_token(Preprocessor* state) {
	Token next_token = {};
	while (true) {
		next_token = tokenizer_next_token(&state->tokenizer);
		if (next_token.kind == TOKEN_HASH) {
			preprocessor_skip_derective(state);
		}
	}

	return next_token;
}

#endif
