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
typedef struct IdentifierScope IdentifierScope;
typedef struct IdentifierNamespace IdentifierNamespace;

bool type_equal(const ParsedType* a, const ParsedType* b);

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
	IDENT_TYPE_DEF = IDENT_CATEGORY_TYPE        | 2,
	IDENT_VARIABLE = IDENT_CATEGORY_VAR_OR_FUNC | 3,
	IDENT_FUNCTION = IDENT_CATEGORY_VAR_OR_FUNC | 4,
} IdentifierEntryKind;

#define IDENT_ENTRY_KIND_COUNT 5
static const uint8_t IDENT_ENTRY_INDEX_MASK = 0b1111;

typedef enum {
	IDENT_NAMESPACE_DEFAULT,
	IDENT_NAMESPACE_TAGGED,
	IDENT_NAMESPACE_ALIAS,
	IDENT_NAMESPACE_COUNT
} IdentifierNamespaceKind;

struct IdentifierEntry {
	SourceString name;
	IdentifierEntryKind kind;
	IdentifierNamespaceKind owner_namespace;

	IdentifierScope* owner_scope;

	IdentifierEntry* next_in_scope;
	IdentifierEntry* prev;

	union {
		ParsedStruct* struct_def;
		ParsedEnum* enum_def;
		ParsedTypeDef* type_def;
		ParsedFunction* function_def;
		ParsedVariable* variable;
	};
};

struct IdentifierScope {
	// Incremented by 1 for each new instance.
	uint64_t id;

	union {
		IdentifierScope* parent;

		// NOTE: Only used for reusing instances in `IdentifierStorage`
		IdentifierScope* next_free;
	};

	IdentifierEntry* first_identifier;
	IdentifierEntry* last_identifier;

	uint32_t nested_entry_count[IDENT_ENTRY_KIND_COUNT];
	uint32_t entry_count[IDENT_ENTRY_KIND_COUNT];
};

struct IdentifierNamespace {
	size_t count;
	size_t capacity;

	// NOTE: Keys and entries are allocated from the same memory region
	String* keys;
	IdentifierEntry** entries;
};

struct IdentifierStorage {
	Arena* allocator;

	Allocator namespace_allocator;
	IdentifierNamespace namespaces[IDENT_NAMESPACE_COUNT];

	uint64_t next_scope_id;
	IdentifierScope* current_scope;

	IdentifierEntry* next_free_entry;
	IdentifierScope* next_free_scope;
};

void ident_storage_init(IdentifierStorage* storage, Allocator namespace_allocator, Arena* allocator);
void ident_storage_release(IdentifierStorage* storage);

IdentifierEntry* ident_storage_find(IdentifierStorage* storage,
		IdentifierNamespaceKind namespace_kind,
		String name);

IdentifierEntry* ident_storage_insert(IdentifierStorage* storage,
		IdentifierNamespaceKind namespace_kind,
		IdentifierEntryKind entry_kind,
		SourceString name);

void ident_storage_remove(IdentifierStorage* storage,
		IdentifierNamespaceKind namespace_kind,
		SourceString name);

IdentifierScope* ident_storage_begin_scope(IdentifierStorage* storage);
void ident_storage_end_scope(IdentifierStorage* storage);

//
// Parser
//

typedef struct {
	Arena* ast_allocator;

	Diagnostics* diagnostics;
	Preprocessor* preprocessor;

	IdentifierStorage* ident_storage;
} Parser;

void parser_init(Parser* parser,
		Arena* ast_allocator,
		IdentifierStorage* ident_storage,
		Preprocessor* preprocessor,
		Diagnostics* diagnostics);

void parser_parse(Parser* parser, ParsedAST* ast);

#endif
