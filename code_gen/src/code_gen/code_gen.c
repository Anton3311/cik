#include "code_gen.h"

//
// SymbolMap
//

static const size_t SYMBOL_MAP_INITIAL_CAPACITY = 32;

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
