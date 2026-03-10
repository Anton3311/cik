#ifndef PARSER_H
#define PARSER_H

#include "core/core.h"
#include "parser/parsed_ast.h"

// A bit unnecessary, a forward declaration would've been enough
#include "parser/tokenizer.h"
#include "parser/diagnostics.h"
#include "parser/preprocessor.h"

typedef struct IdentifierStorage IdentifierStorage;
typedef struct IdentifierEntry IdentifierEntry;

//
// IdentifierStorage
//

typedef enum {
	IDENT_CATEGORY_TYPE        = 1 << 4,
	IDENT_CATEGORY_VAR_OR_FUNC = 2 << 4,
} IdentifierCategory;

typedef enum {
	IDENT_STRUCT =   IDENT_CATEGORY_TYPE        | 0,
	IDENT_ENUM =     IDENT_CATEGORY_TYPE        | 1,
	IDENT_VARIABLE = IDENT_CATEGORY_VAR_OR_FUNC | 2,
	IDENT_FUNCTION = IDENT_CATEGORY_VAR_OR_FUNC | 3,
} IdentifierEntryKind;

struct IdentifierEntry {
	SourceString name;
	IdentifierEntryKind kind;

	IdentifierEntry* prev;

	union {
		ParsedStruct* struct_def;
		ParsedEnum* enum_def;
		ParsedFunction* function_def;
	};
};

struct IdentifierStorage {
	Arena* allocator;

	size_t count;
	size_t capacity;
	IdentifierEntry** entries;

	IdentifierEntry** next_free;
};

void ident_storage_init(IdentifierStorage* storage, Arena* allocator);
IdentifierEntry* ident_storage_find(IdentifierStorage* storage, String name);
IdentifierEntry* ident_storage_insert(IdentifierStorage* storage, SourceString name);
void ident_storage_remove(IdentifierStorage* storage, SourceString name);

typedef struct {
	Arena* ast_allocator;

	Diagnostics* diagnostics;
	Preprocessor* preprocessor;

	IdentifierStorage ident_storage;
} Parser;

void parser_init(Parser* parser,
		Arena* ast_allocator,
		Arena* ident_allocator,
		Preprocessor* preprocessor,
		Diagnostics* diagnostics);

void parser_parse(Parser* parser, ParsedAST* ast);

#endif
