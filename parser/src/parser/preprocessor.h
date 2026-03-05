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
	Token* tokens;
	size_t count;
} MacroArgumentTokens;

typedef struct {
	const MacroDefinition* macro;
	size_t token_index;

	// Which macro call argument is being expanding
	size_t argument_index;
	size_t argument_token_index;

	// Size matches the number of macro parameters
	MacroArgumentTokens* argument_tokens;
	SourceRange call_source_range;
} MacroCallState;

typedef struct {
	String source_path;

	Arena* allocator;
	Arena* temp_allocator;
	Arena* generated_tokens_allocator;
	Diagnostics* diagnostics;
	Tokenizer tokenizer;
	LineInfo line_info;
	MacroTable macro_table;

	MacroCallState* macro_call_stack;
	size_t macro_call_stack_depth;
	size_t macro_call_stack_capacity;

	bool has_pending_next_token;
	Token pending_next_token;
} Preprocessor;

typedef enum {
	MACRO_STYLE_DEFAULT,
	MACRO_STYLE_FUNCTION,
} MacroStyle;

// Tells the preprocessor how to interpret the identifier token
// in the defintion of the macro
typedef enum {
	MACRO_TOKEN_HINT_NONE,
	MACRO_TOKEN_HINT_PARAMETER,
	MACRO_TOKEN_HINT_STRING_OPERATOR,
} MacroTokenHintKind;

typedef struct {
	MacroTokenHintKind kind;
	union {
		struct {
			size_t index;
		} param;
		struct {
			size_t param_index;
		} string_op;
	};
} MacroTokenHint;

typedef enum {
	BUILTIN_MACRO_NONE,
	BUILTIN_MACRO_LINE,
	BUILTIN_MACRO_FILE,
} BuiltinMacroKind;

struct MacroDefinition {
	String name;
	BuiltinMacroKind builtin_kind;

	MacroStyle style;

	String* parameter_names;
	size_t parameter_count;
	
	Token* tokens;
	MacroTokenHint* token_hints;
	size_t token_count;
};

// Returns SIZE_MAX when not found
size_t macro_find_param_by_name(const MacroDefinition* macro, String param_name);

void preprocessor_init(Preprocessor* state,
		String source_path,
		String source_code,
		const LineInfo* line_info,
		Diagnostics* diagnostics,
		Arena* allocator,
		Arena* temp_allocator,
		Arena* generated_tokens_allocator);

void preprocessor_skip_derective(Preprocessor* state);
bool preprocessor_get_next_macro_expantion_token(Preprocessor* state, Token* out_token);

// NOTE: Returns null in case a call to a macro doesn't produce any tokens
MacroCallState* preprocessor_init_macro_call(Preprocessor* state, const MacroDefinition* macro, Token macro_call_ident);

Token preprocessor_view_next(Preprocessor* state);
Token preprocessor_next_token(Preprocessor* state);

#endif
