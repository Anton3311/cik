#include "x64_linker.h"

#include "core/core.h"

static uint64_t _compute_total_program_size_and_func_offsets(const LoweredUnit* units,
		size_t unit_count,
		Arena* allocator,
		LinkedUnitState** out_states) {

	profile_scope_start(__func__);

	LinkedUnitState* states = arena_alloc_array(allocator, LinkedUnitState, unit_count);
	for (size_t unit_index = 0; unit_index < unit_count; unit_index += 1) {
		states[unit_index].func_offsets = arena_alloc_array(allocator,
				uint64_t,
				units[unit_index].function_count);
	}

	uint64_t size = 0;
	for (size_t unit_index = 0; unit_index < unit_count; unit_index += 1) {
		LoweredUnit unit = units[unit_index];
		LinkedUnitState unit_state = states[unit_index];
		for (size_t i = 0; i < unit.function_count; i += 1) {
			unit_state.func_offsets[i] = size;
			size += unit.functions[i].size_in_bytes;
		}
	}

	*out_states = states;
	profile_scope_end();
	return size;
}

static void _copy_code(const LoweredUnit lowered_unit,
		const LinkedUnitState unit_state,
		MachineCodeBuffer machine_code) {
	profile_scope_start(__func__);

	for (size_t i = 0; i < lowered_unit.function_count; i += 1) {
		const LoweredFunction* func = &lowered_unit.functions[i];
		uint64_t function_offset = unit_state.func_offsets[i];
		memcpy((uint8_t*)machine_code.code + function_offset, func->code, func->size_in_bytes);
	}

	profile_scope_end();
}

typedef struct {
	size_t count;
	size_t capacity;

	String* keys;
	size_t* unit_indices;
	SymbolId* symbols;
} GlobalSymbolMap;

static bool _insert_global_symbol(GlobalSymbolMap* map,
		String name,
		size_t unit_index,
		SymbolId symbol) {

	size_t hash = hash_string(name);
	for (size_t i = 0; i < map->capacity; i += 1) {
		size_t index = (hash + i) % map->capacity;

		if (map->keys[index].v == NULL) {
			map->keys[index] = name;
			map->unit_indices[index] = unit_index;
			map->symbols[index] = symbol;

			map->count += 1;
			return true;
		}
	}

	return false;
}

static size_t _find_global_symbol(const GlobalSymbolMap* map, String name) {
	size_t hash = hash_string(name);
	for (size_t i = 0; i < map->capacity; i += 1) {
		size_t index = (hash + i) % map->capacity;

		if (str_equal(map->keys[index], name)) {
			return index;
		}

		if (map->keys[index].v == NULL) {
			return SIZE_MAX;
		}
	}

	return SIZE_MAX;
}

static GlobalSymbolMap _collect_global_symbols(const SymbolMap* symbol_maps,
		size_t unit_count,
		Arena* allocator) {

	profile_scope_start(__func__);

	size_t global_symbol_count = 0;

	for (size_t i = 0; i < unit_count; i += 1) {
		const SymbolMap* map = &symbol_maps[i];
		size_t symbol_count = map->count;
		for (size_t symbol_index = 0; symbol_index < symbol_count; symbol_index += 1) {
			const Symbol* symbol = &map->symbols[symbol_index];

			if (symbol->linkage == SYMBOL_LINKAGE_EXTERNAL_STATIC) {
				global_symbol_count += 1;
			}
		}
	}

	GlobalSymbolMap map = {};
	map.count = 0;
	map.capacity = global_symbol_count * 100 / 80;
	map.keys = arena_alloc_array_zeroed(allocator, String, map.capacity);
	map.unit_indices = arena_alloc_array(allocator, size_t, map.capacity);
	map.symbols = arena_alloc_array(allocator, SymbolId, map.capacity);

	memset(map.unit_indices, 0xff, sizeof(*map.unit_indices) * map.capacity);
	memset(map.symbols, 0xff, sizeof(*map.symbols) * map.capacity);

	for (size_t i = 0; i < unit_count; i += 1) {
		const SymbolMap* symbol_map = &symbol_maps[i];
		size_t symbol_count = symbol_map->count;
		for (size_t symbol_index = 0; symbol_index < symbol_count; symbol_index += 1) {
			const Symbol* symbol = &symbol_map->symbols[symbol_index];

			if (symbol->linkage == SYMBOL_LINKAGE_EXTERNAL_STATIC) {
				bool inserted = _insert_global_symbol(&map, symbol->name, i, (SymbolId)symbol_index);
				assert(inserted);
			}
		}
	}

	profile_scope_end();
	return map;
}

static bool _link_unit(size_t lowered_unit_index,
		const LoweredUnit lowered_unit,
		const SymbolMap* imported_symbol_maps,
		const SymbolMap* exported_symbol_maps,
		const SymbolMap* dynamically_linked_symbols,
		const LinkedUnitState* unit_states,
		const GlobalSymbolMap* global_symbols,
		MachineCodeBuffer machine_code) {
	profile_scope_start(__func__);

	bool result = true;
	const SymbolMap* imported_symbol_map = &imported_symbol_maps[lowered_unit_index];

	for (size_t i = 0; i < lowered_unit.function_count; i += 1) {
		const LoweredFunction* func = &lowered_unit.functions[i];

		void* function_offset = (uint8_t*)machine_code.code + unit_states[lowered_unit_index].func_offsets[i];

		size_t placeholder_count = func->call_addr_placeholder_count;
		for (size_t placeholder_index = 0;
				placeholder_index < placeholder_count;
				placeholder_index += 1) {

			CallAddressPlaceholder placeholder = func->call_addr_placeholders[placeholder_index];
			
			const Symbol* callee_symbol = &imported_symbol_map->symbols[placeholder.function_index];

			uint64_t callee_address = UINT64_MAX;
			uint64_t instruction_end_offset = (uint64_t)placeholder.instruction_end_offset + (uint64_t)function_offset;

			switch (callee_symbol->linkage) {
			case SYMBOL_LINKAGE_INTERNAL: {
				// Link against an exported symbol in the same unit.
				const SymbolMap* exported_symbols = &exported_symbol_maps[lowered_unit_index];
				SymbolId callee_impl_id = symbol_map_find(
						exported_symbols,
						symbol_key_from_symbol(callee_symbol));

				if (callee_impl_id == SYMBOL_ID_INVALID) {
					printf("unresolved reference to '%.*s'\n", STR_FMT(callee_symbol->name));
					result = false;
					continue;
				}

				const Symbol* callee_impl = &exported_symbols->symbols[callee_impl_id];
				
				uint64_t callee_offset = unit_states[lowered_unit_index]
					.func_offsets[callee_impl->data.func_index];

				callee_address = (uint64_t)machine_code.code + callee_offset;
				break;
			}
			case SYMBOL_LINKAGE_EXTERNAL_STATIC: {
				size_t entry_index = _find_global_symbol(global_symbols, callee_symbol->name);
				if (entry_index == SIZE_MAX) {
					printf("unresolved reference to '%.*s'\n", STR_FMT(callee_symbol->name));
					result = false;
					continue;
				}

				SymbolId callee_impl_id = global_symbols->symbols[entry_index];
				size_t unit_index = global_symbols->unit_indices[entry_index];

				const Symbol* callee_impl = &exported_symbol_maps[unit_index].symbols[callee_impl_id];
				uint32_t function_index = callee_impl->data.func_index;

				uint64_t callee_offset = unit_states[unit_index].func_offsets[function_index];
				callee_address = (uint64_t)machine_code.code + callee_offset;
				break;
			}
			case SYMBOL_LINKAGE_EXTERNAL_DYNAMIC: {
				SymbolId symbol_impl_id = symbol_map_find(
						dynamically_linked_symbols,
						symbol_key_from_symbol(callee_symbol));

				if (symbol_impl_id == SYMBOL_ID_INVALID) {
					printf("unresolved reference to '%.*s'\n", STR_FMT(callee_symbol->name));
					result = false;
					continue;
				}

				const Symbol* symbol_impl = &dynamically_linked_symbols->symbols[symbol_impl_id];
				callee_address = (uint64_t)symbol_impl->linkage_data.external_dynamic.impl;
				break;
			}
			}

			void* addr_offset = (uint8_t*)function_offset + placeholder.addr_offset;

			switch (placeholder.kind) {
			case CALL_ADDR_ABSOLUTE: {
				memcpy(addr_offset, &callee_address, sizeof(callee_address));
				break;
			}
			case CALL_ADDR_RELATIVE: {
				assert(callee_address <= INT64_MAX);
				assert(instruction_end_offset <= INT64_MAX);

				int64_t relative_offset = (int64_t)callee_address - (int64_t)instruction_end_offset;
				assert(INT32_MIN <= relative_offset && relative_offset <= INT32_MAX);

				int32_t relative_offset_32 = (int32_t)relative_offset;

				memcpy(addr_offset, &relative_offset_32, sizeof(relative_offset_32));
				break;
			}
			}
		}
	}

	profile_scope_end();
	return result;
}

bool linker_link(const LoweredUnit* units,
		const SymbolMap* imported_symbol_maps,
		const SymbolMap* exported_symbol_maps,
		const SymbolMap* dynamically_linked_symbols,
		size_t unit_count,
		String entry_point_name,
		Arena* allocator,
		LinkedProgram* out_linked) {
	profile_scope_start(__func__);

	LinkedUnitState* unit_states = NULL;
	size_t code_size = _compute_total_program_size_and_func_offsets(units,
			unit_count,
			allocator,
			&unit_states);

	MachineCodeBuffer machine_code = {};
	machine_code.size_in_bytes = code_size;
	machine_code.code = allocate_executable(code_size);

	for (size_t i = 0; i < unit_count; i += 1) {
		_copy_code(units[i], unit_states[i], machine_code);
	}

	ArenaRegion temp = arena_begin_temp(allocator);

	GlobalSymbolMap global_symbols = _collect_global_symbols(exported_symbol_maps,
			unit_count,
			allocator);

	bool result = true;
	for (size_t i = 0; i < unit_count; i += 1) {
		result = result && _link_unit(i,
				units[i],
				imported_symbol_maps,
				exported_symbol_maps,
				dynamically_linked_symbols,
				unit_states,
				&global_symbols,
				machine_code);
	}

	void* entry_point_address = NULL;

	{
		size_t entry_index = _find_global_symbol(&global_symbols, entry_point_name);
		if (entry_index == SIZE_MAX) {
			printf("unresolved reference to '%.*s'\n", STR_FMT(entry_point_name));
			result = false;
		} else {
			SymbolId entry_point_impl_id = global_symbols.symbols[entry_index];
			size_t unit_index = global_symbols.unit_indices[entry_index];

			const Symbol* entry_point_impl = &exported_symbol_maps[unit_index].symbols[entry_point_impl_id];
			uint32_t function_index = entry_point_impl->data.func_index;

			uint64_t entry_point_offset = unit_states[unit_index].func_offsets[function_index];
			entry_point_address = (uint8_t*)machine_code.code + entry_point_offset;
		}
	}

	arena_end_temp(temp);

	out_linked->machine_code = machine_code;
	out_linked->unit_count = unit_count;
	out_linked->unit_states = unit_states;
	out_linked->entry_point_address = entry_point_address;

	profile_scope_end();
	return result;
}
