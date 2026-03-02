#ifndef PREPROCESSOR_H
#define PREPROCESSOR_H

#include "parser/tokenizer.h"
#include "parser/source_info.h"
#include "parser/diagnostics.h"

typedef struct MacroDefinition MacroDefinition;

typedef struct {
	size_t capacity;
	size_t count;
	MacroDefinition* macros;
} MacroTable;

void macro_table_append(MacroTable* table, const MacroDefinition* macro);
const MacroDefinition* macro_table_find(const MacroTable* table, String name);

//
// Preprocessor
//

typedef struct {
	const MacroDefinition* macro;
	size_t token_index;
} MacroCallState;

typedef struct {
	Arena* allocator;
	Diagnostics* diagnostics;
	Tokenizer tokenizer;
	LineInfo line_info;
	MacroTable macro_table;

	MacroCallState* macro_call_stack;
	size_t macro_call_stack_depth;
	size_t macro_call_stack_capacity;
} Preprocessor;

typedef enum {
	MACRO_STYLE_DEFAULT,
	MACRO_STYLE_FUNCTION,
} MacroStyle;

struct MacroDefinition {
	String name;

	MacroStyle style;

	String* parameter_names;
	size_t parameter_count;
	
	Token* tokens;
	size_t token_count;
};

void preprocessor_init(Preprocessor* state,
		String source_code,
		const LineInfo* line_info,
		Diagnostics* diagnostics,
		Arena* allocator);

void preprocessor_skip_derective(Preprocessor* state);

inline Token preprocessor_next_token(Preprocessor* state) {
	if (state->macro_call_stack_depth > 0) {
		// NOTE: Inside an unwrapping macro call.
		//       Instead of getting tokens from the tokenizer
		//       return the onces from the macro.
		//
		//       In that way the call gets replaced with whatever code was defined in the macro.
		MacroCallState* macro_call = NULL;

		// NOTE: Pop finished macro calls off of the stack
		while (state->macro_call_stack_depth > 0) {
			macro_call = &state->macro_call_stack[state->macro_call_stack_depth - 1];
			if (macro_call->token_index == macro_call->macro->token_count) {
				state->macro_call_stack_depth -= 1;
				macro_call = NULL;
			} else {
				break;
			}
		}

		if (macro_call) {
			Token next_token = macro_call->macro->tokens[macro_call->token_index];
			macro_call->token_index += 1;
			return next_token;
		}
	}

	Token next_token = {};
	while (true) {
		next_token = tokenizer_next_token(&state->tokenizer);
		if (next_token.kind == TOKEN_HASH) {
			preprocessor_skip_derective(state);
		} else if (next_token.kind == TOKEN_IDENT) {
			const MacroDefinition* macro = macro_table_find(&state->macro_table, next_token.string);
			if (macro) {
				assert(macro->style == MACRO_STYLE_DEFAULT);
				if (macro->token_count == 0) {
					break;
				}

				assert(state->macro_call_stack_depth < state->macro_call_stack_capacity);

				MacroCallState* macro_call = &state->macro_call_stack[state->macro_call_stack_depth];
				state->macro_call_stack_depth += 1;

				macro_call->macro = macro;
				macro_call->token_index = 1; // first token is returned immediately
				return macro->tokens[0];
			}

			break;
		} else {
			break;
		}
	}

	return next_token;
}

#endif
