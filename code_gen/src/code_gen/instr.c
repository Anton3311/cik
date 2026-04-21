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

	[INSTR_BRANCH] = INSTR_FEATURE_CONTROL,
	[INSTR_JUMP] = INSTR_FEATURE_CONTROL,

	[INSTR_RETURN_VALUE] = INSTR_FEATURE_CONTROL,

	[INSTR_REGION] = 0,
};

static String instr_kind_to_string[] = {
	[INSTR_NO_OP] = STR_LIT("no_op"),
	[INSTR_CONST_8] = STR_LIT("const_8"),
	[INSTR_CONST_16] = STR_LIT("const_16"),
	[INSTR_CONST_32] = STR_LIT("const_32"),
	[INSTR_CONST_64] = STR_LIT("const_64"),
	[INSTR_BIN_OP_8] = STR_LIT("bin_op_8"),
	[INSTR_BIN_OP_16] = STR_LIT("bin_op_16"),
	[INSTR_BIN_OP_32] = STR_LIT("bin_op_32"),
	[INSTR_BIN_OP_64] = STR_LIT("bin_op_64"),
	[INSTR_BRANCH] = STR_LIT("branch"),
	[INSTR_JUMP] = STR_LIT("jump"),
	[INSTR_RETURN_VALUE] = STR_LIT("return_value"),
	[INSTR_REGION] = STR_LIT("region"),
};

static String s_instr_bin_op_kind_to_string[] = {
	[INSTR_BIN_ADD] = STR_LIT("add"),
	[INSTR_BIN_SUB] = STR_LIT("sub"),
	[INSTR_BIN_MUL] = STR_LIT("mul"),
	[INSTR_BIN_DIV] = STR_LIT("div"),
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

void instr_enumerate_dependencies(const InstrBuffer buffer,
		InstrIndex instr_index,
		InstrStack* out_dependencies) {

	const Instr* instr = &buffer.instr[instr_index.value];
	switch (instr->kind) {
	case INSTR_NO_OP:
		break;

	case INSTR_CONST_8:
	case INSTR_CONST_16:
	case INSTR_CONST_32:
	case INSTR_CONST_64:
		break;

	case INSTR_BIN_OP_8:
	case INSTR_BIN_OP_16:
	case INSTR_BIN_OP_32:
	case INSTR_BIN_OP_64:
		instr_stack_push(out_dependencies, instr->bin_op.left);
		instr_stack_push(out_dependencies, instr->bin_op.right);
		break;

	case INSTR_BRANCH:
		instr_stack_push(out_dependencies, instr->branch.condition);
		instr_stack_push(out_dependencies, instr->branch.true_region);
		instr_stack_push(out_dependencies, instr->branch.false_region);
		break;

	case INSTR_JUMP:
		instr_stack_push(out_dependencies, instr->jump.target_region);
		break;

	case INSTR_RETURN_VALUE:
		instr_stack_push(out_dependencies, instr->ret.value);
		break;

	case INSTR_REGION:
		instr_stack_push(out_dependencies, instr->region.last_instr);
		break;

	case INSTR_COUNT:
		unreachable();
	}
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
			InstrIndex dep_index = stack.instr[i];
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

#if 0
InstrUsageGroupArray instr_group_by_overlapping_usages(const InstrBuffer buffer,
		const InstrUsageRange* usage_ranges,
		Arena* allocator,
		Arena* temp_allocator) {

	ArenaRegion temp = arena_begin_temp(temp_allocator);
	InstrUsageRange* ranges_copy = arena_alloc_array(temp_allocator, InstrUsageRange, buffer.count);
	array_copy(ranges_copy, usage_ranges, buffer.count);

	InstrIndex* instr_for_usage_range = arena_alloc_array(temp_allocator, InstrIndex, buffer.count);
	for (size_t i = 0; i < buffer.count; i += 1) {
		instr_for_usage_range[i].value = (uint16_t)i;
	}

	for (size_t i = 0; i < buffer.count; i += 1) {
		for (size_t j = i + 1; j < buffer.count; j += 1) {
			InstrUsageRange a = ranges_copy[j - 1];
			InstrUsageRange b = ranges_copy[j];

			InstrIndex instr_a = instr_for_usage_range[j - 1];
			InstrIndex instr_b = instr_for_usage_range[j];

			bool should_swap = (a.first_instr.value < b.first_instr.value)
				|| (a.first_instr.value == b.first_instr.value
					&& a.last_instr.value < b.last_instr.value);

			if (should_swap) {
				ranges_copy[j - 1] = b;
				ranges_copy[j] = a;

				instr_for_usage_range[j - 1] = instr_b;
				instr_for_usage_range[j] = instr_a;
			}
		}
	}

	InstrUsageGroupArray usage_groups;
	usage_groups.groups = arena_alloc_array(temp_allocator, InstrUsageGroup, 0);
	usage_groups.count = 0;

	{
		InstrUsageRange current_range = ranges_copy[0];
		InstrUsageGroup current_group = {};
		current_group.instr = arena_alloc_array(allocator, InstrIndex, 1);
		current_group.count = 1;
		current_group.instr[0] = instr_for_usage_range[0];

		for (size_t i = 1; i < buffer.count; i += 1) {
			InstrUsageRange range = ranges_copy[i];

			if (range.first_intsr.value <= range.last_instr.value) {
				current_range.last_instr = range.last_instr;

				arena_alloc(allocator, InstrIndex);
				current_group.instr[current_group.count] = instr_for_usage_range[i];
			} else {

			}
		}
	}

	arena_end_temp(temp);
}
#endif

String instr_name(InstrKind instr_kind) {
	return instr_kind_to_string[instr_kind];
}

void instr_print(const Instr* instr) {
	String name = instr_name(instr->kind);

	size_t name_width = 12;

	printf("\033[32;1m%.*s\033[0m \033[%uC", STR_FMT(name), (uint32_t)(name_width - name.length));

	switch (instr->kind) {
	case INSTR_NO_OP:
		break;

	case INSTR_CONST_8:
		printf("%u %d", (uint32_t)instr->const_8.u8, (int32_t)instr->const_8.i8);
		break;
	case INSTR_CONST_16:
		printf("%u %d", (uint32_t)instr->const_16.u16, (int32_t)instr->const_16.i16);
		break;
	case INSTR_CONST_32:
		printf("%u %d %f",
				instr->const_32.u32,
				instr->const_32.i32,
				instr->const_32.f32);
		break;
	case INSTR_CONST_64:
		printf("%llu %lld %f",
				instr->const_64.i64,
				instr->const_64.u64,
				instr->const_64.f64);
		break;
	case INSTR_BRANCH:
		printf("condition: %u true: %u false: %u",
				(uint32_t)instr->branch.condition.value,
				(uint32_t)instr->branch.true_region.value,
				(uint32_t)instr->branch.false_region.value);
		break;
	case INSTR_JUMP:
		printf("%u", (uint32_t)instr->jump.target_region.value);
		break;
	case INSTR_RETURN_VALUE:
		printf("%u", (uint32_t)instr->ret.value.value);
		break;
	case INSTR_REGION:
		printf("%u", (uint32_t)instr->region.last_instr.value);
		break;
	case INSTR_BIN_OP_8:
	case INSTR_BIN_OP_16:
	case INSTR_BIN_OP_32:
	case INSTR_BIN_OP_64: {
		printf("op: %.*s left: %u right: %u",
				STR_FMT(s_instr_bin_op_kind_to_string[instr->bin_op.kind]),
				(uint32_t)instr->bin_op.left.value,
				(uint32_t)instr->bin_op.right.value);
		break;
	}
	}

	printf("\n");
}

void instr_print_all(InstrBuffer instr_buffer) {
	for (size_t i = 0; i < instr_buffer.count; i += 1) {
		printf("%zu\033[12G", i);
		instr_print(&instr_buffer.instr[i]);
	}
}
