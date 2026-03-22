#ifndef PREPROCESSOR_H
#define PREPROCESSOR_H

#include "parser/tokenizer.h"
#include "parser/source_info.h"
#include "parser/diagnostics.h"

typedef struct MacroDefinition MacroDefinition;
typedef struct PreprocessorBranchState PreprocessorBranchState;

typedef struct {
	size_t capacity;
	size_t count;
	MacroDefinition* macros;
} MacroTable;

void macro_table_append(MacroTable* table, const MacroDefinition* macro);
bool macro_table_remove(MacroTable* table, String name);
const MacroDefinition* macro_table_find(const MacroTable* table, String name);

//
// Preprocessor
//

typedef struct {
	Token* tokens;
	size_t count;
} MacroArgumentTokens;

typedef enum {
	MACRO_CALL_TOKEN,
	MACRO_CALL_ARGUMENT_EXPANSION,
	MACRO_CALL_VA_ARGS_EXPANSION,
} MacroCallState;

typedef struct {
	// Which macro call argument is being expanded
	size_t arg_index;
	size_t arg_token_index;
} MacroCallArgExpansion;

typedef struct {
	size_t arg_index;
	size_t arg_token_index;
} MacroCallVaArgsExpansion;

typedef struct {
	const MacroDefinition* macro;
	size_t token_index;

	MacroCallState state;

	union {
		MacroCallArgExpansion arg_expansion;
	};

	// Size matches the number of macro parameters
	MacroArgumentTokens* argument_tokens;
	size_t argument_count;
	SourceRange call_source_range;
} MacroCall;

typedef enum {
	DIRECTIVE_INCLUDE,
	DIRECTIVE_DEFINE,
	DIRECTIVE_UNDEF,
	DIRECTIVE_IF,
	DIRECTIVE_ELIF,
	DIRECTIVE_ELSE,
	DIRECTIVE_ENDIF,
	DIRECTIVE_IFDEF,
	DIRECTIVE_IFNDEF,
	DIRECTIVE_PRAGMA,
	DIRECTIVE_ERROR,
} DirectiveKind;

String directive_kind_to_string(DirectiveKind kind);

typedef struct {
	DirectiveKind kind;
	SourceRange source_range;
} ParsedDirective;

struct PreprocessorBranchState {
	bool predicate_value;
	bool has_enabled_alternative_branch;
	ParsedDirective current_directive; // are we in an #if, #elif or #else etc block?

	PreprocessorBranchState* parent;
};

typedef struct {
	MacroCall* frames;
	size_t depth;
	size_t capacity;
} MacroCallStack;

typedef struct {
	String source_path;

	Arena* allocator;
	Arena* temp_allocator;
	Arena* generated_tokens_allocator;
	Diagnostics* diagnostics;
	Tokenizer tokenizer;
	LineInfo line_info;
	MacroTable macro_table;
	MacroCallStack macro_call_stack;

	bool has_pending_next_token;
	Token pending_next_token;

	PreprocessorBranchState* current_branch_state;
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
	MACRO_TOKEN_HINT_TOKEN_INSERT_OPERATOR,
	MACRO_TOKEN_HINT_VA_ARGS,
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
		
		struct {
			size_t param_index;
		} token_insert_op;
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

	bool has_va_args;
	
	Token* tokens;
	MacroTokenHint* token_hints;
	size_t token_count;
};

void preprocessor_init(Preprocessor* state,
		String source_path,
		String source_code,
		const LineInfo* line_info,
		Diagnostics* diagnostics,
		Arena* allocator,
		Arena* temp_allocator,
		Arena* generated_tokens_allocator);

Token preprocessor_view_next(Preprocessor* state);
Token preprocessor_next_token(Preprocessor* state);

#endif
