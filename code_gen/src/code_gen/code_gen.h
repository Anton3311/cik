#ifndef CODE_GEN_H
#define CODE_GEN_H

#include "core/core.h"

typedef enum {
	FUNCTION_IMPL_NONE,
	FUNCTION_IMPL_INTERNAL,
	FUNCTION_IMPL_EXTERNAL,
} FunctionImplKind;

typedef struct {
	String name;
	
	FunctionImplKind impl_kind;

	union {
		const void* external_address;

		struct {
			size_t function_index;
			size_t compilation_unit_index;
		} internal;
	};
} FunctionRef;

// Supports only insertion and lookup
typedef struct {
	Allocator allocator;

	FunctionRef* refs;
	uint16_t size;
	uint16_t capacity;
} FunctionRefTable;

// Returns entry index in the hash map. If not found returns `UINT16_MAX`
uint16_t func_ref_table_entry_index(const FunctionRefTable* table, String name);
bool func_ref_table_resolve_ref_to(FunctionRefTable* table, String name, void* impl_address);
uint16_t func_ref_table_insert(FunctionRefTable* table, String name);
void func_ref_table_release(FunctionRefTable* table);

inline uint16_t func_ref_table_get_or_insert(FunctionRefTable* table, String name) {
	uint16_t index = func_ref_table_entry_index(table, name);
	if (index == UINT16_MAX) {
		return func_ref_table_insert(table, name);
	}

	return index;
}

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

// Returns `SYMBOL_ID_INVALID`, on failure
SymbolId symbol_map_find(const SymbolMap* map, SymbolKey key);

#endif
