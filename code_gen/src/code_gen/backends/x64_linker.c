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

static void _link_unit(size_t lowered_unit_index,
		const LoweredUnit lowered_unit,
		const LinkedUnitState* unit_states,
		const FunctionRefTable* ref_table,
		MachineCodeBuffer machine_code) {
	profile_scope_start(__func__);

	for (size_t i = 0; i < lowered_unit.function_count; i += 1) {
		const LoweredFunction* func = &lowered_unit.functions[i];

		void* function_offset = (uint8_t*)machine_code.code + unit_states[lowered_unit_index].func_offsets[i];
		memcpy(function_offset, func->code, func->size_in_bytes);

		size_t placeholder_count = func->call_addr_placeholder_count;
		for (size_t placeholder_index = 0;
				placeholder_index < placeholder_count;
				placeholder_index += 1) {

			CallAddressPlaceholder placeholder = func->call_addr_placeholders[placeholder_index];
			
			uint64_t instruction_end_offset = (uint64_t)placeholder.instruction_end_offset + (uint64_t)function_offset;

			const FunctionRef* callee = &ref_table->refs[placeholder.function_index];
			uint64_t callee_address;

			switch (callee->impl_kind) {
			case FUNCTION_IMPL_INTERNAL: {
				size_t unit_index = callee->internal.compilation_unit_index;
				size_t function_index = callee->internal.function_index;
				uint64_t callee_offset = unit_states[unit_index].func_offsets[function_index];
				callee_address = (uint64_t)machine_code.code + callee_offset;
				break;
			}
			case FUNCTION_IMPL_EXTERNAL:
				callee_address = (uint64_t)callee->external_address;
				break;
			case FUNCTION_IMPL_NONE:
				printf("unresolved reference to '%.*s'\n", STR_FMT(callee->name));
				unreachable();
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
}

LinkedProgram linker_link(const LoweredUnit* units,
		size_t unit_count,
		const FunctionRefTable* ref_table,
		Arena* allocator) {
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
		_link_unit(i, units[i], unit_states, ref_table, machine_code);
	}

	LinkedProgram linked = {};
	linked.machine_code = machine_code;
	linked.unit_count = unit_count;
	linked.unit_states = unit_states;

	profile_scope_end();
	return linked;
}

uint64_t linker_resolve_func_address(const LinkedProgram* linked_program,
		const FunctionRefTable* ref_table,
		String name) {
	profile_scope_start(__func__);

	uint16_t ref_id = func_ref_table_entry_index(ref_table, name);
	if (ref_id == UINT16_MAX) {
		profile_scope_end();
		return UINT64_MAX;
	} 

	const FunctionRef* ref = &ref_table->refs[ref_id];

	size_t unit_index = ref->internal.compilation_unit_index;
	LinkedUnitState unit_state = linked_program->unit_states[unit_index];

	profile_scope_end();
	return unit_state.func_offsets[ref->internal.function_index];
}
