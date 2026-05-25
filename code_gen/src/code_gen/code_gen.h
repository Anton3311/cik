#ifndef CODE_GEN_H
#define CODE_GEN_H

#include "core/core.h"

typedef struct {
	String name;
	const void* address;
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

#endif
