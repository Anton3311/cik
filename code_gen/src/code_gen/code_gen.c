#include "code_gen.h"

//
// SymbolMap
//

static const size_t SYMBOL_MAP_INITIAL_CAPACITY = 8;

// Returns `SIZE_MAX` when the key already exists
inline size_t _find_symbol_insert_index(const SymbolKey* keys, SymbolKey key, size_t capacity) {
	profile_scope_start(__func__);
	size_t hash = hash_string(key.name);

	for (size_t i = 0; i < capacity; i += 1) {
		size_t index = (hash + i) % capacity;

		if (keys[index].name.v == NULL) {
			profile_scope_end();
			return index;
		} else if (keys[index].linkage == key.linkage && str_equal(keys[index].name, key.name)) {
			profile_scope_end();
			return SIZE_MAX;
		}
	}

	unreachable();
	return SIZE_MAX;
}

static void _symbol_map_grow(SymbolMap* map) {
	profile_scope_start(__func__);
	size_t new_capacity = max(SYMBOL_MAP_INITIAL_CAPACITY, map->capacity * 2);

	// Reallocate the array

	Symbol* new_symbols = allocator_alloc_array(map->allocator, Symbol, new_capacity);

	if (map->symbols) {
		memcpy(new_symbols, map->symbols, map->count * sizeof(*new_symbols));
		allocator_release(map->allocator, map->symbols);
	}

	// Free the hash map, since it will be rebuilt from scratch
	if (map->keys) {
		allocator_release(map->allocator, map->keys);
	}

	if (map->values) {
		allocator_release(map->allocator, map->values);
	}

	// Reallocate the hash map

	size_t new_map_entry_count = new_capacity * 100 / 80;

	SymbolKey* new_keys = allocator_alloc_array(map->allocator, SymbolKey, new_map_entry_count);
	SymbolId* new_values = allocator_alloc_array(map->allocator, SymbolId, new_map_entry_count);

	memset(new_keys, 0, sizeof(*new_keys) * new_map_entry_count);
	memset(new_values, 0xff, sizeof(*new_values) * new_map_entry_count);

	size_t symbol_count = map->count;
	for (size_t i = 0; i < symbol_count; i += 1) {
		SymbolKey key = symbol_key_from_symbol(&new_symbols[i]);
		size_t insert_index = _find_symbol_insert_index(new_keys, key, new_map_entry_count);

		assert(insert_index != SIZE_MAX);

		new_values[insert_index] = (SymbolId)i;
		new_keys[insert_index] = key;
	}

	map->symbols = new_symbols;
	map->keys = new_keys;
	map->values = new_values;
	map->capacity = new_capacity;
	map->map_capacity = new_map_entry_count;

	profile_scope_end();
}

void symbol_map_init(SymbolMap* map, Allocator allocator) {
	map->allocator = allocator;

	map->count = 0;
	map->capacity = 0;
	map->symbols = NULL;

	map->map_capacity = 0;
	map->keys = NULL;
	map->values = NULL;
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

SymbolId symbol_map_insert(SymbolMap* map, const Symbol* symbol) {
	profile_scope_start(__func__);

	if (map->count == map->capacity) {
		_symbol_map_grow(map);
	}

	SymbolKey key = symbol_key_from_symbol(symbol);
	size_t insert_index = _find_symbol_insert_index(map->keys, key, map->map_capacity);

	if (insert_index == SIZE_MAX) {
		profile_scope_end();
		return SYMBOL_ID_INVALID;
	}

	SymbolId id = (SymbolId)map->count;

	map->symbols[id] = *symbol;
	map->values[insert_index] = id;
	map->keys[insert_index] = key;
	map->count += 1;
	profile_scope_end();
	return id;
}

SymbolId symbol_map_insert_dynamically_linked_impl(SymbolMap* map, String name, void* impl) {
	Symbol symbol;
	memset(&symbol, 0xff, sizeof(symbol));

	symbol.name = name;
	symbol.linkage = SYMBOL_LINKAGE_EXTERNAL_DYNAMIC;
	symbol.linkage_data.external_dynamic.impl = impl;

	SymbolId id = symbol_map_insert(map, &symbol);
	assert(id != SYMBOL_ID_INVALID);
	return id;
}

SymbolId symbol_map_find(const SymbolMap* map, SymbolKey key) {
	profile_scope_start(__func__);

	size_t hash = hash_string(key.name);
	for (size_t i = 0; i < map->map_capacity; i += 1) {
		size_t index = (hash + i) % map->map_capacity;

		if (map->keys[index].name.v == NULL) {
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
