#ifndef PREPROCESSOR_H
#define PREPROCESSOR_H

#include "parser/tokenizer.h"
#include "parser/source_info.h"
#include "parser/diagnostics.h"

//
// Preprocessor
//

typedef struct {
	Arena* allocator;
	Diagnostics* diagnostics;
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
		} else {
			break;
		}
	}

	return next_token;
}

#endif
