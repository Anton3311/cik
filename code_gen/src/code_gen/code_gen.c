#include "code_gen.h"

uint16_t func_ref_table_entry_index(const FunctionRefTable* table, String name) {
	for (uint16_t i = 0; i < table->size; i += 1) {
		if (str_equal(table->refs[i].name, name)) {
			return i;
		}
	}

	return UINT16_MAX;
}

bool func_ref_table_resolve_ref_to(FunctionRefTable* table, String name, void* impl_address) {
	assert(impl_address != NULL);

	uint16_t entry_index = func_ref_table_entry_index(table, name);
	if (entry_index == UINT16_MAX) {
		return false;
	}

	FunctionRef* ref = &table->refs[entry_index];
	if (ref->address != NULL) {
		return false;
	}

	ref->address = impl_address;
	return true;
}

uint16_t func_ref_table_insert(FunctionRefTable* table, String name) {
	assert(func_ref_table_entry_index(table, name) == UINT16_MAX);

	if (table->size == table->capacity) {
		uint16_t new_capacity = max(4, table->capacity + table->capacity / 2);
		FunctionRef* new_array = allocator_alloc_array(table->allocator, FunctionRef, new_capacity);

		if (table->size > 0) {
			assert(table->refs);
			array_copy(new_array, table->refs, table->size);

			allocator_release(table->allocator, table->refs);
		} else {
			assert(table->refs == NULL);
		}

		table->refs = new_array;
		table->capacity = new_capacity;
	}

	assert(table->capacity > 0);

	assert(table->size < UINT16_MAX);
	FunctionRef* ref = &table->refs[table->size];
	uint16_t id = table->size;

	table->size += 1;

	ref->name = name;
	ref->address = NULL;
	return id;
}

void func_ref_table_release(FunctionRefTable* table) {
	allocator_release(table->allocator, table->refs);
	*table = (FunctionRefTable) {};
}
