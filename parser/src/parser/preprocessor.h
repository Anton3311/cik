#ifndef PREPROCESSOR_H
#define PREPROCESSOR_H

#include "parser/tokenizer.h"

typedef struct {
	Tokenizer tokenizer;
} Preprocessor;

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
