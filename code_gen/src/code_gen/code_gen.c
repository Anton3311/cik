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
	ref->impl_kind = FUNCTION_IMPL_NONE;
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

//
// SymbolMap
//

static const size_t SYMBOL_MAP_INITIAL_CAPACITY = 16;

inline size_t _symbol_look_up_by_name_and_linkage(const SymbolMap* map, String name) {
	return SIZE_MAX;
}

static void _symbol_map_grow(SymbolMap* map) {
	
}

void symbol_map_init(SymbolMap* map, Allocator allocator) {
	profile_scope_start(__func__);

	map->allocator = allocator;

	map->count = 0;
	map->capacity = SYMBOL_MAP_INITIAL_CAPACITY;
	map->symbols = allocator_alloc_array(allocator, Symbol, SYMBOL_MAP_INITIAL_CAPACITY);

	size_t map_entry_count = map->capacity * 100 / 80;
	map->map_capacity = map_entry_count;
	map->keys = allocator_alloc_array(allocator, SymbolKey, map_entry_count);
	map->values = allocator_alloc_array(allocator, uint32_t, map_entry_count);

	memset(map->keys, 0, sizeof(*map->keys) * map_entry_count);

	profile_scope_end();
}

void symbol_map_release(SymbolMap* map) {
	profile_scope_start(__func__);

	if (map->symbols) {
		allocator_release(map->allocator, map->symbols);
	}

	if (map->keys) {
		allocator_release(map->allocator, map->keys);
	}

	if (map->values) {
		allocator_release(map->allocator, map->values);
	}

	memset(map, 0, sizeof(*map));
	profile_scope_end();
}

inline bool _symbol_key_equal(const SymbolKey a, const SymbolKey b) {
	if (a.linkage != b.linkage) {
		return false;
	}

	return str_equal(a.name, b.name);
}

SymbolId symbol_map_insert(SymbolMap* map, const Symbol* symbol) {
	profile_scope_start(__func__);
	assert(map->count < map->capacity);

	size_t hash = hash_string(symbol->name);

	for (size_t i = 0; i < map->map_capacity; i += 1) {
		size_t index = (hash + i) % map->map_capacity;

		if (map->keys[index].name.v == NULL) {
			SymbolId id = (SymbolId)map->count;

			map->symbols[id] = *symbol;
			map->values[index] = id;
			map->keys[index] = (SymbolKey) {
				.name = symbol->name,
				.linkage = symbol->linkage,
			};

			map->count += 1;
			profile_scope_end();
			return id;
		} else if (_symbol_key_equal(map->keys[index], symbol_key_from_symbol(symbol))) {
			profile_scope_end();
			return SYMBOL_ID_INVALID;
		}
	}

	profile_scope_end();
	return SYMBOL_ID_INVALID;
}

SymbolId symbol_map_find(const SymbolMap* map, SymbolKey key) {
	profile_scope_start(__func__);

	size_t hash = hash_string(key.name);
	for (size_t i = 0; i < map->map_capacity; i += 1) {
		size_t index = (hash + i) % map->map_capacity;

		if (map->symbols[index].name.v == NULL) {
			profile_scope_end();
			return SYMBOL_ID_INVALID;
		}

		if (_symbol_key_equal(map->keys[index], key)) {
			profile_scope_end();
			return map->values[index];
		}
	}

	profile_scope_end();
	return SYMBOL_ID_INVALID;
}
