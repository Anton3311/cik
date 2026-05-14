#include "instr.h"

InstrFeatureFlag INSTR_FEATURES[INSTR_COUNT] = {
	[INSTR_NO_OP] = 0,

	[INSTR_CONST_8] = INSTR_FEATURE_REG_STORAGE,
	[INSTR_CONST_16] = INSTR_FEATURE_REG_STORAGE,
	[INSTR_CONST_32] = INSTR_FEATURE_REG_STORAGE,
	[INSTR_CONST_64] = INSTR_FEATURE_REG_STORAGE,

	[INSTR_BIN_OP_8] = INSTR_FEATURE_REG_STORAGE,
	[INSTR_BIN_OP_16] = INSTR_FEATURE_REG_STORAGE,
	[INSTR_BIN_OP_32] = INSTR_FEATURE_REG_STORAGE,
	[INSTR_BIN_OP_64] = INSTR_FEATURE_REG_STORAGE,

	[INSTR_PTR_LOAD_8] = INSTR_FEATURE_REG_STORAGE,
	[INSTR_PTR_LOAD_16] = INSTR_FEATURE_REG_STORAGE,
	[INSTR_PTR_LOAD_32] = INSTR_FEATURE_REG_STORAGE,
	[INSTR_PTR_LOAD_64] = INSTR_FEATURE_REG_STORAGE,

	[INSTR_LOGICAL_SHIFT_LEFT_8] = INSTR_FEATURE_REG_STORAGE,
	[INSTR_LOGICAL_SHIFT_LEFT_16] = INSTR_FEATURE_REG_STORAGE,
	[INSTR_LOGICAL_SHIFT_LEFT_32] = INSTR_FEATURE_REG_STORAGE,
	[INSTR_LOGICAL_SHIFT_LEFT_64] = INSTR_FEATURE_REG_STORAGE,

	[INSTR_LOGICAL_SHIFT_RIGHT_8] = INSTR_FEATURE_REG_STORAGE,
	[INSTR_LOGICAL_SHIFT_RIGHT_16] = INSTR_FEATURE_REG_STORAGE,
	[INSTR_LOGICAL_SHIFT_RIGHT_32] = INSTR_FEATURE_REG_STORAGE,
	[INSTR_LOGICAL_SHIFT_RIGHT_64] = INSTR_FEATURE_REG_STORAGE,

	[INSTR_COMPARE_8] = INSTR_FEATURE_REG_STORAGE,
	[INSTR_COMPARE_16] = INSTR_FEATURE_REG_STORAGE,
	[INSTR_COMPARE_32] = INSTR_FEATURE_REG_STORAGE,
	[INSTR_COMPARE_64] = INSTR_FEATURE_REG_STORAGE,

	[INSTR_LOAD_ARG] = INSTR_FEATURE_REG_STORAGE,

	[INSTR_BRANCH] = INSTR_FEATURE_CONTROL,
	[INSTR_JUMP] = INSTR_FEATURE_CONTROL,

	[INSTR_RETURN_VALUE] = INSTR_FEATURE_CONTROL,

	[INSTR_CALL_INTERNAL] = INSTR_FEATURE_CONTROL | INSTR_FEATURE_REG_STORAGE,

	[INSTR_IO_STATE] = INSTR_FEATURE_CONTROL,
	[INSTR_REGION] = INSTR_FEATURE_CONTROL,
};

bool instr_region_finished(const InstrBuffer* buffer, InstrIndex region_index) {
	const Instr* instr = instr_buffer_at(buffer, region_index);
	InstrIndex last_instr_in_region = instr->region.last_instr;

	if (last_instr_in_region.value == INVALID_INSTR_INDEX.value) {
		return false;
	}

	InstrKind last_instr_kind = buffer->instr[last_instr_in_region.value].kind;
	return has_flag(INSTR_FEATURES[last_instr_kind], INSTR_FEATURE_CONTROL);
}

InstrUsageRange* instr_compute_usage_ranges(const InstrBuffer buffer,
		InstrIndex root_instr,
		Arena* allocator,
		Arena* temp_allocator) {

	ArenaRegion temp = arena_begin_temp(temp_allocator);

	InstrUsageRange* usage_ranges = arena_alloc_array(allocator, InstrUsageRange, buffer.count);
	memset(usage_ranges, 0xff, sizeof(*usage_ranges) * buffer.count);

	InstrStack stack;
	instr_stack_alloc(&stack, temp_allocator, buffer.count);

	BitArray visited_instr = bit_array_alloc(temp_allocator, buffer.count);
	bit_array_clear(&visited_instr);

	instr_stack_push(&stack, root_instr);

	while (stack.count) {
		InstrIndex instr_index = instr_stack_pop(&stack);
		if (instr_index.value == UINT16_MAX) {
			continue;
		}

		if (bit_array_get(&visited_instr, instr_index.value)) {
			// This instruction has already been visited
			continue;
		}

		bit_array_set(&visited_instr, instr_index.value, true);
		if (instr_usage_range_is_valid(usage_ranges[instr_index.value])) {
			usage_ranges[instr_index.value] = instr_usage_range_extended(usage_ranges[instr_index.value], instr_index);
		} else if (instr_usage_range_is_empty(usage_ranges[instr_index.value])) {
			usage_ranges[instr_index.value].first_usage = instr_index;
			usage_ranges[instr_index.value].last_usage = instr_index;
		} else {
			panic("InstrUsageRange has invalid state");
		}

		size_t first_dep_index = stack.count;
		instr_enumerate_dependencies(buffer, instr_index, &stack);
		size_t dep_count = stack.count - first_dep_index;

		for (size_t i = first_dep_index; i < first_dep_index + dep_count; i += 1) {
			// Visit dependencies in reverse order of them being pushed onto the stack
			InstrIndex dep_index = stack.instr[stack.count - 1 - i];
			if (dep_index.value >= buffer.count) {
				continue;
			}

			InstrUsageRange usage_range = usage_ranges[dep_index.value];
			if (instr_usage_range_is_valid(usage_range)) {
				usage_range = instr_usage_range_extended(usage_range, instr_index);
			} else if (instr_usage_range_is_empty(usage_range)) {
				usage_range.first_usage = instr_index;
				usage_range.last_usage = instr_index;
			} else {
				panic("InstrUsageRange has invalid state");
			}

			usage_ranges[dep_index.value] = usage_range;
		}
	}

	arena_end_temp(temp);
	return usage_ranges;
}

void instr_print_all(InstrBuffer instr_buffer) {
	for (size_t i = 0; i < instr_buffer.count; i += 1) {
		printf("%zu\033[12G", i);
		instr_print(&instr_buffer.instr[i]);
	}
}
