#include "code_gen.h"

uint16_t func_ref_table_entry_index(const FunctionRefTable* table, String name) {
	profile_scope_start(__func__);
	for (uint16_t i = 0; i < table->size; i += 1) {
		if (str_equal(table->refs[i].name, name)) {
			profile_scope_end();
			return i;
		}
	}

	profile_scope_end();
	return UINT16_MAX;
}

bool func_ref_table_resolve_ref_to(FunctionRefTable* table, String name, void* impl_address) {
	profile_scope_start(__func__);
	assert(impl_address != NULL);

	uint16_t entry_index = func_ref_table_entry_index(table, name);
	if (entry_index == UINT16_MAX) {
		profile_scope_end();
		return false;
	}

	FunctionRef* ref = &table->refs[entry_index];
	if (ref->external_address != NULL) {
		profile_scope_end();
		return false;
	}

	ref->impl_kind = FUNCTION_IMPL_EXTERNAL;
	ref->external_address = impl_address;
	profile_scope_end();
	return true;
}

uint16_t func_ref_table_insert(FunctionRefTable* table, String name) {
	profile_scope_start(__func__);
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
	ref->external_address = NULL;
	profile_scope_end();
	return id;
}

void func_ref_table_release(FunctionRefTable* table) {
	if (table->refs != NULL) {
		allocator_release(table->allocator, table->refs);
	}
	*table = (FunctionRefTable) {};
}
