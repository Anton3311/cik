#include "x64_reg_alloc.h"

#include "core/profiler.h"

// Defined in `x64.c`
extern X64InstrStorageRequirement s_instr_storage_requiremenets[INSTR_COUNT];

static InstrIndexArray _gather_instr_with_storage_requirement(const InstrBuffer* instr_buffer,
		const InstrUsageRange* live_ranges,
		Arena* allocator) {

	InstrIndexArray result;
	result.count = 0;
	result.instr = arena_alloc_array(allocator, InstrIndex, 0);

	for (size_t i = 0; i < instr_buffer->count; i += 1) {
		const InstrKind kind = instr_buffer->instr[i].kind;
		if (!has_flag(INSTR_FEATURES[kind], INSTR_FEATURE_REG_STORAGE)) {
			continue;
		}

		if (live_ranges[i].value == UINT32_MAX) {
			// Not used
			continue;
		}

		arena_alloc(allocator, InstrIndex);
		result.instr[result.count].value = (uint16_t)i;
		result.count += 1;
	}

	return result;
}

// Returned array stores an array of edges for each instruction in `instr_with_storage_requirement`
// The array must be indexed using an element index of the `instr_with_storage_requirement`
static UInt16Array* _build_interference_graph(const InstrIndexArray instr_with_storage_requirement,
		const InstrUsageRange* live_ranges,
		Arena* allocator) {

	// Each array stores indices into `instr_with_storage_requirement`
	UInt16Array* graph_edges = arena_alloc_array_zeroed(allocator,
			UInt16Array,
			instr_with_storage_requirement.count);

	for (size_t i = 0; i < instr_with_storage_requirement.count; i += 1) {
		UInt16Array* edges = &graph_edges[i];
		edges->values = arena_alloc_array(allocator, uint16_t, 0);
	
		InstrUsageRange live_range_a = live_ranges[instr_with_storage_requirement.instr[i].value];
		for (size_t j = 0; j < instr_with_storage_requirement.count; j += 1) {
			if (i == j) {
				continue;
			}

			InstrUsageRange live_range_b = live_ranges[instr_with_storage_requirement.instr[j].value];

			uint16_t max_start = max(live_range_a.first_usage.value, live_range_b.first_usage.value);
			uint16_t min_end = min(live_range_a.last_usage.value, live_range_b.last_usage.value);

			bool overlap = min_end > max_start;
			if (overlap) {
				arena_alloc(allocator, InstrIndex);
				edges->values[edges->count] = (uint16_t)j;
				edges->count += 1;
			}
		}
	}

	return graph_edges;
}

// Writes storage locations into `instr_storage` array.
// This array must be of size `instr_buffer.count`
static void _run_graph_coloring(const InstrBuffer* instr_buffer,
		InstrIndexArray instr_with_storage_requirement,
		UInt16Array* interference_graph,
		InstrStorageLocation* instr_storage,
		uint16_t allowed_registers,
		Arena* temp_allocator) {
	profile_scope_start(__func__);
	ArenaRegion temp = arena_begin_temp(temp_allocator);

	uint16_t* potential_instr_registers = arena_alloc_array(temp_allocator,
			uint16_t,
			instr_buffer->count);

	for (size_t i = 0; i < instr_buffer->count; i += 1) {
		InstrKind kind = instr_buffer->instr[i].kind;

		if (kind == INSTR_LOAD_ARG) {
			continue;
		}

		if (has_flag(INSTR_FEATURES[kind], INSTR_FEATURE_REG_STORAGE)) {
			uint16_t instr_registers = s_instr_storage_requiremenets[kind].allowed_registers;
			potential_instr_registers[i] = instr_registers & allowed_registers;

			assert_msg(potential_instr_registers[i] != 0,
					"This instruction must be spilled, but spilling is not yet implemented");
		}
	}

	// Assign locations to function arguments
	for (size_t i = 0; i < instr_with_storage_requirement.count; i += 1) {
		InstrIndex instr_index = instr_with_storage_requirement.instr[i];
		InstrKind kind = instr_buffer->instr[instr_index.value].kind;

		if (kind != INSTR_LOAD_ARG) {
			continue;
		}

		const Instr* instr = &instr_buffer->instr[instr_index.value];

		// NOTE: INSTR_LOAD_ARG are handled separtely here.
		//       Since these instructions access arguments which
		//       are stored in the `cdecl_arg_regs`

		// NOTE: Well that's slowly turning into a mess, why is it here?
		//       Probably need to introduce a proper concept of calling
		//       conventions on the code gen level
		X64Register cdecl_arg_regs[] = { X64_REG_C, X64_REG_D, X64_REG_8, X64_REG_9 };
		assert(instr->load_arg.index < array_size(cdecl_arg_regs));

		X64Register reg = cdecl_arg_regs[instr->load_arg.index];
		instr_storage[instr_index.value].kind = INSTR_STORAGE_REG;
		instr_storage[instr_index.value].reg = reg;

		UInt16Array edges = interference_graph[i];
		for (size_t j = 0; j < edges.count; j += 1) {
			InstrIndex interfering_instr = instr_with_storage_requirement.instr[edges.values[j]];
			potential_instr_registers[interfering_instr.value] &= ~(1 << reg);
		}
	}

	// Assign locations to the rest of the instructions
	for (size_t i = 0; i < instr_with_storage_requirement.count; i += 1) {
		InstrIndex instr_index = instr_with_storage_requirement.instr[i];

		if (instr_storage[instr_index.value].kind != INSTR_STORAGE_NONE) {
			continue;
		}

		uint16_t potential_registers = potential_instr_registers[instr_index.value];
		assert_msg(potential_registers != 0,
				"This instruction must be spilled, but spilling is not yet implemented");

		uint16_t first_potential_register = count_trailing_zeros(potential_registers);
		assert(first_potential_register < 16);

		instr_storage[instr_index.value].kind = INSTR_STORAGE_REG;
		instr_storage[instr_index.value].reg = first_potential_register;

		UInt16Array edges = interference_graph[i];
		for (size_t j = 0; j < edges.count; j += 1) {
			InstrIndex interfering_instr = instr_with_storage_requirement.instr[edges.values[j]];
			potential_instr_registers[interfering_instr.value] &= ~(1 << first_potential_register);
		}
	}

	arena_end_temp(temp);
	profile_scope_end();
}

InstrStorageLocation* x64_alloc_regs(const InstrBuffer* instr_buffer,
		InstrUsageRange* live_ranges,
		uint16_t allowed_registers,
		Arena* allocator,
		Arena* temp_allocator) {
	profile_scope_start(__func__);
	ArenaRegion temp = arena_begin_temp(temp_allocator);

	InstrIndexArray instr_with_storage_requirement = _gather_instr_with_storage_requirement(
			instr_buffer,
			live_ranges,
			temp_allocator);

	UInt16Array* interference_graph = _build_interference_graph(instr_with_storage_requirement,
			live_ranges,
			temp_allocator);

	InstrStorageLocation* storage_locations = arena_alloc_array_zeroed(allocator,
			InstrStorageLocation,
			instr_buffer->count);

	_run_graph_coloring(instr_buffer,
			instr_with_storage_requirement,
			interference_graph,
			storage_locations,
			allowed_registers,
			temp_allocator);

	arena_end_temp(temp);
	profile_scope_end();

	return storage_locations;
}
