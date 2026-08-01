#include "x64_reg_alloc.h"

// Defined in `x64.c`
extern X64InstrStorageRequirement s_instr_storage_requiremenets[INSTR_COUNT];

static InstrIndexArray _gather_instr_with_storage_requirement(const InstrBuffer* instr_buffer,
		const InstrLiveRange* live_ranges,
		Arena* allocator) {
	profile_scope_start(__func__);

	InstrIndexArray result;
	result.count = 0;
	result.instr = arena_alloc_array(allocator, InstrIndex, 0);

	for (size_t i = 0; i < instr_buffer->count; i += 1) {
		if (live_ranges[i].value == UINT32_MAX) {
			// Not used
			continue;
		}

		const InstrKind kind = instr_buffer->instr[i].kind;
		if (has_flag(INSTR_FEATURES[kind], INSTR_FEATURE_REG_STORAGE)) {
			arena_alloc(allocator, InstrIndex);
			result.instr[result.count].value = (uint16_t)i;
			result.count += 1;
		}

		if (has_flag(INSTR_FEATURES[kind], INSTR_FEATURE_STACK_STORAGE)) {
			arena_alloc(allocator, InstrIndex);
			result.instr[result.count].value = (uint16_t)i;
			result.count += 1;
		}
	}

	profile_scope_end();
	return result;
}

// Returned array stores an array of edges for each instruction in `instr_with_storage_requirement`
//
// The array must be indexed using an element index of the `instr_with_storage_requirement`
static UInt16Array* _build_interference_graph(const InstrIndexArray instr_with_storage_requirement,
		const InstrLiveRange* live_ranges,
		Arena* allocator) {

	profile_scope_start(__func__);

	// Each array stores indices into `instr_with_storage_requirement`
	UInt16Array* graph_edges = arena_alloc_array_zeroed(allocator,
			UInt16Array,
			instr_with_storage_requirement.count);

	for (size_t i = 0; i < instr_with_storage_requirement.count; i += 1) {
		UInt16Array* edges = &graph_edges[i];
		edges->values = arena_alloc_array(allocator, uint16_t, 0);
	
		InstrLiveRange live_range_a = live_ranges[instr_with_storage_requirement.instr[i].value];
		for (size_t j = 0; j < instr_with_storage_requirement.count; j += 1) {
			if (i == j) {
				continue;
			}

			InstrLiveRange live_range_b = live_ranges[instr_with_storage_requirement.instr[j].value];

			uint16_t max_start = max(live_range_a.start, live_range_b.start);
			uint16_t min_end = min(live_range_a.end, live_range_b.end);

			// Check whether live ranges overlap.
			bool overlap = min_end >= max_start;

#if 1
			// If the ranges only overlap at their ends, then don't consider them overlapping
			if (live_range_a.end == live_range_b.start) {
				overlap = false;
			}

			if (live_range_a.start == live_range_b.end) {
				overlap = false;
			}
#endif

			if (overlap) {
				arena_alloc(allocator, InstrIndex);
				edges->values[edges->count] = (uint16_t)j;
				edges->count += 1;
			}
		}
	}

	profile_scope_end();
	return graph_edges;
}

inline bool _instr_allowed_to_share_a_register(InstrLiveRange live_range_a,
		InstrLiveRange live_range_b) {
	if (live_range_a.end == live_range_b.start) {
		return true;
	}

	if (live_range_a.start == live_range_b.end) {
		return true;
	}

	return false;
}

// Writes storage locations into `instr_storage` array.
//
// This array is expected to be of size `instr_buffer.count`
static void _run_graph_coloring(const InstrBuffer* instr_buffer,
		const InstrLiveRange* live_ranges,
		InstrIndexArray instr_with_storage_requirement,
		UInt16Array* interference_graph,
		uint16_t allowed_registers,
		Arena* allocator,
		Arena* temp_allocator,
		RegisterAllocationResult* out_result) {
	profile_scope_start(__func__);
	ArenaRegion temp = arena_begin_temp(temp_allocator);

	InstrStorageLocation* instr_storage = arena_alloc_array_zeroed(allocator,
			InstrStorageLocation,
			instr_buffer->count);

	uint16_t* potential_instr_registers = arena_alloc_array(temp_allocator,
			uint16_t,
			instr_buffer->count);

	for (size_t i = 0; i < instr_buffer->count; i += 1) {
		InstrKind kind = instr_buffer->instr[i].kind;

		if (kind == INSTR_LOAD_ARG_8
				|| kind == INSTR_LOAD_ARG_16
				|| kind == INSTR_LOAD_ARG_32
				|| kind == INSTR_LOAD_ARG_64) {
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

		if (kind != INSTR_LOAD_ARG_8
				&& kind != INSTR_LOAD_ARG_16
				&& kind != INSTR_LOAD_ARG_32
				&& kind != INSTR_LOAD_ARG_64) {
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

	uint32_t stack_offset = 0;
	for (size_t i = 0; i < instr_with_storage_requirement.count; i += 1) {
		InstrIndex instr_index = instr_with_storage_requirement.instr[i];

		if (instr_storage[instr_index.value].kind != INSTR_STORAGE_NONE) {
			continue;
		}

		const Instr* instr = instr_buffer_at(instr_buffer, instr_index);
		if (has_flag(INSTR_FEATURES[instr->kind], INSTR_FEATURE_STACK_STORAGE)) {
			assert(instr->stack_alloc.alignment > 0);
			assert(is_power_of_2(instr->stack_alloc.alignment));

			stack_offset = align(stack_offset, instr->stack_alloc.alignment);

			instr_storage[instr_index.value].kind = INSTR_STORAGE_STACK;
			instr_storage[instr_index.value].stack.offset = stack_offset;
			stack_offset += instr->stack_alloc.size;
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

	out_result->allocations = instr_storage;
	out_result->stack_usage = stack_offset;

	arena_end_temp(temp);
	profile_scope_end();
}

RegisterAllocationResult x64_alloc_regs(const InstrBuffer* instr_buffer,
		InstrLiveRange* live_ranges,
		uint16_t allowed_registers,
		Arena* allocator,
		Arena* temp_allocator) {
	profile_scope_start(__func__);

	InstrIndexArray instr_with_storage_requirement = _gather_instr_with_storage_requirement(
			instr_buffer,
			live_ranges,
			temp_allocator);

	UInt16Array* interference_graph = _build_interference_graph(instr_with_storage_requirement,
			live_ranges,
			temp_allocator);

	RegisterAllocationResult result;
	result.instr_with_storage_requirement = instr_with_storage_requirement;
	result.interference_graph = interference_graph;

	_run_graph_coloring(instr_buffer,
			live_ranges,
			instr_with_storage_requirement,
			interference_graph,
			allowed_registers,
			allocator,
			temp_allocator,
			&result);

	profile_scope_end();
	return result;
}
