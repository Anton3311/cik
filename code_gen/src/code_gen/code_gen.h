#ifndef CODE_GEN_H
#define CODE_GEN_H

#include "core/core.h"

//
// SymbolMap
//

typedef uint32_t SymbolId;

#define SYMBOL_ID_INVALID ((SymbolId)UINT32_MAX)

typedef enum {
	SYMBOL_FUNCTION,
	SYMBOL_VARIABLE,
} SymbolKind;

typedef enum {
	SYMBOL_SCOPE_UNIT,
	SYMBOL_SCOPE_GLOBAL,
} SymbolScope;

typedef enum {
	SYMBOL_LINKAGE_INTERNAL,
	SYMBOL_LINKAGE_EXTERNAL_STATIC,
	SYMBOL_LINKAGE_EXTERNAL_DYNAMIC,
} SymbolLinkage;

typedef struct {
	String name;
	SymbolLinkage linkage;
} SymbolKey;

typedef struct {
	String name;
	SymbolLinkage linkage;

	union {
		// SYMBOL_FUNCTION
		uint32_t func_index;

		// TODO: Add `SYMBOL_VARIABLE` data when implementing globals
	} data;

	union {
		struct {
			uint32_t compilation_unit_index;
		} internal;

		struct {
			void* impl;
		} external_dynamic;
	} linkage_data;
} Symbol;

inline SymbolKey symbol_key_from_symbol(const Symbol* symbol) {
	return (SymbolKey) {
		.name = symbol->name,
		.linkage = symbol->linkage,
	};
}

typedef struct {
	Symbol* symbols;
	size_t count;
	size_t capacity;

	SymbolKey* keys;
	SymbolId* values;
	size_t map_capacity;

	Allocator allocator;
} SymbolMap;

void symbol_map_init(SymbolMap* map, Allocator allocator);
void symbol_map_release(SymbolMap* map);

// Returns `SYMBOL_ID_INVALID`, on failure
SymbolId symbol_map_insert(SymbolMap* map, const Symbol* symbol);

SymbolId symbol_map_insert_dynamically_linked_impl(SymbolMap* map, String name, void* impl);

// Returns `SYMBOL_ID_INVALID`, on failure
SymbolId symbol_map_find(const SymbolMap* map, SymbolKey key);

#endif
