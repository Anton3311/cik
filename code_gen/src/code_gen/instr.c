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

	[INSTR_CAST_TO_8] = INSTR_FEATURE_REG_STORAGE,
	[INSTR_CAST_TO_16] = INSTR_FEATURE_REG_STORAGE,
	[INSTR_CAST_TO_32] = INSTR_FEATURE_REG_STORAGE,
	[INSTR_CAST_TO_64] = INSTR_FEATURE_REG_STORAGE,

	[INSTR_LOAD_ARG] = INSTR_FEATURE_REG_STORAGE,

	[INSTR_BRANCH] = INSTR_FEATURE_CONTROL,
	[INSTR_JUMP] = INSTR_FEATURE_CONTROL,

	[INSTR_RETURN_VALUE] = INSTR_FEATURE_CONTROL,

	[INSTR_CALL_INTERNAL] = INSTR_FEATURE_CONTROL | INSTR_FEATURE_REG_STORAGE,

	[INSTR_PHI] = INSTR_FEATURE_REG_STORAGE,
	[INSTR_SELECT] = INSTR_FEATURE_NONE,

	[INSTR_IO_STATE] = INSTR_FEATURE_CONTROL,
	[INSTR_REGION] = INSTR_FEATURE_CONTROL,
};

InstrIndex instr_new_cast(InstrBuffer* buffer,
		Arena* allocator,
		InstrIndex value,
		uint8_t target_bit_count) {
	assert(is_power_of_2(target_bit_count));
	assert(target_bit_count >= 8);
	assert(target_bit_count <= 64);

	uint8_t sub_kind_index = count_trailing_zeros(target_bit_count >> 3);

	InstrIndex i = instr_buffer_append(buffer, allocator);
	Instr* instr = instr_buffer_at(buffer, i);
	instr->kind = INSTR_CAST_TO_8 + sub_kind_index;
	instr->cast.value = value;
	return i;
}

uint16_t instr_region_id(const InstrBuffer* buffer, InstrIndex region_index) {
	const Instr* instr = &buffer->instr[region_index.value];
	assert(instr->kind == INSTR_REGION);

	uint16_t id = instr->region.id;
	assert(id < buffer->region_count);
	return id;
}

bool instr_region_finished(const InstrBuffer* buffer, InstrIndex region_index) {
	const Instr* instr = instr_buffer_at(buffer, region_index);
	InstrIndex last_instr_in_region = instr->region.last_instr;

	if (last_instr_in_region.value == INVALID_INSTR_INDEX.value) {
		return false;
	}

	InstrKind last_instr_kind = buffer->instr[last_instr_in_region.value].kind;
	return has_flag(INSTR_FEATURES[last_instr_kind], INSTR_FEATURE_CONTROL);
}

void instr_push_input_dependeices(const InstrBuffer* buffer,
		InstrInputs inputs,
		InstrStack* out_dependencies) {

	for (uint16_t i = 0; i < inputs.count; i += 1) {
		instr_stack_push(out_dependencies, buffer->inputs_buffer[inputs.start + i]);
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

		// NOTE: The loop after this check is used to extend the usage range of this instruction dependencies.
		//       In this way data dependencies get defined for the later register allocation step.
		//       However some instructions are only used to specify an order dependency,
		//       one of them is `INSTR_IO_STATE`.
		InstrKind this_instr_kind = buffer.instr[instr_index.value].kind;
		if (this_instr_kind == INSTR_IO_STATE) {
			// Don't define any data dependencies
			continue;
		}

		for (size_t i = first_dep_index; i < stack.count; i += 1) {
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

InstrIndexArray _x64_gather_regions_in_bfs_order(const InstrBuffer instr_buffer,
		Arena* allocator,
		Arena* temp_allocator,
		InstrIndex start_region) {

	ArenaRegion temp = arena_begin_temp(temp_allocator);

	BitArray visited = bit_array_alloc(temp_allocator, instr_buffer.count);
	bit_array_clear(&visited);

	InstrQueue queue = {};
	instr_queue_alloc(&queue, temp_allocator, instr_buffer.count);
	instr_queue_push_back(&queue, start_region);

	InstrIndexArray bfs_order;
	bfs_order.instr = arena_alloc_array(allocator, InstrIndex, 0);
	bfs_order.count = 0;

	bit_array_set(&visited, start_region.value, true);

	while (queue.count > 0) {
		InstrIndex region_index = instr_queue_pop_front(&queue);
		const Instr* region = &instr_buffer.instr[region_index.value];
		assert(region->kind == INSTR_REGION);
		assert(bit_array_get(&visited, region_index.value));

		arena_alloc(allocator, InstrIndex);
		bfs_order.instr[bfs_order.count] = region_index;
		bfs_order.count += 1;

		const Instr* last_instr = &instr_buffer.instr[region->region.last_instr.value];
		switch (last_instr->kind) {
		case INSTR_BRANCH: {
			InstrIndex regions[] = {
				last_instr->branch.true_region,
				last_instr->branch.false_region,
			};

			for (size_t j = 0; j < array_size(regions); j += 1) {
				if (!bit_array_get(&visited, regions[j].value)) {
					bit_array_set(&visited, regions[j].value, true);
					instr_queue_push_back(&queue, regions[j]);
				}
			}

			break;
		}
		case INSTR_JUMP: {
			InstrIndex region = last_instr->jump.target_region;
			if (!bit_array_get(&visited, region.value)) {
				bit_array_set(&visited, region.value, true);
				instr_queue_push_back(&queue, region);
			}
			break;
		}
		case INSTR_RETURN_VALUE:
			break;
		default:
			unreachable();
		}
	}

	arena_end_temp(temp);
	return bfs_order;
}


String instr_format_input_instrs(const InstrIndex* input_instr_buffer,
		InstrInputs inputs,
		Arena* temp_allocator) {
	StringBuilder builder = { .arena = temp_allocator };

	str_builder_append_char(&builder, '[');
	for (uint16_t i = 0; i < inputs.count; i += 1) {
		InstrIndex input = input_instr_buffer[inputs.start + i];
		str_builder_append_int(&builder, input.value);

		if (i != inputs.count - 1) {
			str_builder_append(&builder, STR_LIT(", "));
		}
	}
	str_builder_append_char(&builder, ']');

	return builder.string;
}

void instr_print_all(InstrBuffer instr_buffer, Arena* temp_allocator) {
	for (size_t i = 0; i < instr_buffer.count; i += 1) {
		ArenaRegion temp = arena_begin_temp(temp_allocator);

		printf("%zu", i);
		printf("\033[12G");
		instr_print(&instr_buffer.instr[i], instr_buffer.inputs_buffer, temp_allocator);

		arena_end_temp(temp);
	}
}

void instr_replace_dead_instr(const InstrBuffer instr_buffer, const InstrUsageRange* usage_ranges) {
	for (size_t i = 0; i < instr_buffer.count; i += 1) {
		if (usage_ranges[i].value == UINT32_MAX) {
			instr_buffer.instr[i].kind = INSTR_NO_OP;
		}
	}
}
