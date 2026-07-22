#include "instr.h"

bool instr_bin_op_is_commutative(InstrBinOp op) {
	switch (op) {
	case INSTR_BIN_ADD:
	case INSTR_BIN_IMUL:
	case INSTR_BIN_UMUL:
		return true;
	case INSTR_BIN_SUB:
	case INSTR_BIN_IDIV:
	case INSTR_BIN_UDIV:
	case INSTR_BIN_IMOD:
	case INSTR_BIN_UMOD:
		return false;
	case INSTR_BIN_AND:
	case INSTR_BIN_OR:
	case INSTR_BIN_XOR:
		return true;
	case INSTR_BIN_SHIFT_LEFT:
	case INSTR_BIN_SHIFT_RIGHT:
		return false;
	}

	unreachable();
	return false;
}

InstrFeatureFlag INSTR_FEATURES[INSTR_COUNT] = {
	[INSTR_NO_OP] = 0,

	[INSTR_UNINITIALIZED_8] = INSTR_FEATURE_REG_STORAGE,
	[INSTR_UNINITIALIZED_16] = INSTR_FEATURE_REG_STORAGE,
	[INSTR_UNINITIALIZED_32] = INSTR_FEATURE_REG_STORAGE,
	[INSTR_UNINITIALIZED_64] = INSTR_FEATURE_REG_STORAGE,

	[INSTR_CONST_8] = INSTR_FEATURE_REG_STORAGE,
	[INSTR_CONST_16] = INSTR_FEATURE_REG_STORAGE,
	[INSTR_CONST_32] = INSTR_FEATURE_REG_STORAGE,
	[INSTR_CONST_64] = INSTR_FEATURE_REG_STORAGE,

	[INSTR_CONST_STRING] = INSTR_FEATURE_REG_STORAGE,

	[INSTR_BIN_OP_8] = INSTR_FEATURE_REG_STORAGE,
	[INSTR_BIN_OP_16] = INSTR_FEATURE_REG_STORAGE,
	[INSTR_BIN_OP_32] = INSTR_FEATURE_REG_STORAGE,
	[INSTR_BIN_OP_64] = INSTR_FEATURE_REG_STORAGE,

	[INSTR_NEGATE_8] = INSTR_FEATURE_REG_STORAGE,
	[INSTR_NEGATE_16] = INSTR_FEATURE_REG_STORAGE,
	[INSTR_NEGATE_32] = INSTR_FEATURE_REG_STORAGE,
	[INSTR_NEGATE_64] = INSTR_FEATURE_REG_STORAGE,

	[INSTR_PTR_LOAD_8] = INSTR_FEATURE_REG_STORAGE,
	[INSTR_PTR_LOAD_16] = INSTR_FEATURE_REG_STORAGE,
	[INSTR_PTR_LOAD_32] = INSTR_FEATURE_REG_STORAGE,
	[INSTR_PTR_LOAD_64] = INSTR_FEATURE_REG_STORAGE,

	[INSTR_PTR_STORE_8] = 0,
	[INSTR_PTR_STORE_16] = 0,
	[INSTR_PTR_STORE_32] = 0,
	[INSTR_PTR_STORE_64] = 0,

	[INSTR_BITWISE_NOT_8] = INSTR_FEATURE_REG_STORAGE,
	[INSTR_BITWISE_NOT_16] = INSTR_FEATURE_REG_STORAGE,
	[INSTR_BITWISE_NOT_32] = INSTR_FEATURE_REG_STORAGE,
	[INSTR_BITWISE_NOT_64] = INSTR_FEATURE_REG_STORAGE,

	[INSTR_COMPARE_8] = 0,
	[INSTR_COMPARE_16] = 0,
	[INSTR_COMPARE_32] = 0,
	[INSTR_COMPARE_64] = 0,

	[INSTR_BOOL_TO_INT] = INSTR_FEATURE_REG_STORAGE,

	[INSTR_CAST_TO_8] = INSTR_FEATURE_REG_STORAGE,
	[INSTR_CAST_TO_16] = INSTR_FEATURE_REG_STORAGE,
	[INSTR_CAST_TO_32] = INSTR_FEATURE_REG_STORAGE,
	[INSTR_CAST_TO_64] = INSTR_FEATURE_REG_STORAGE,

	[INSTR_LOAD_ARG_8] = INSTR_FEATURE_REG_STORAGE,
	[INSTR_LOAD_ARG_16] = INSTR_FEATURE_REG_STORAGE,
	[INSTR_LOAD_ARG_32] = INSTR_FEATURE_REG_STORAGE,
	[INSTR_LOAD_ARG_64] = INSTR_FEATURE_REG_STORAGE,

	[INSTR_BRANCH] = INSTR_FEATURE_CONTROL,
	[INSTR_JUMP] = INSTR_FEATURE_CONTROL,

	[INSTR_RETURN_VALUE] = INSTR_FEATURE_CONTROL,
	[INSTR_RET] = INSTR_FEATURE_CONTROL,

	[INSTR_CALL_INDIRECT] = INSTR_FEATURE_CONTROL | INSTR_FEATURE_REG_STORAGE,

	[INSTR_PHI] = INSTR_FEATURE_REG_STORAGE,
	[INSTR_SELECT] = INSTR_FEATURE_NONE,

	[INSTR_IO_STATE] = INSTR_FEATURE_CONTROL,
	[INSTR_REGION] = INSTR_FEATURE_CONTROL,
};

//
// InstrBuffer
//

void instr_buffer_init(InstrBuffer* buffer, Arena* allocator) {
	buffer->instr = arena_alloc_array(allocator, Instr, 0);
	buffer->inputs_buffer = NULL;
	buffer->count = 0;
	buffer->inputs_buffer_size = 0;
	buffer->inputs_buffer_capacity = 0;
	buffer->region_count = 0;
}

void instr_buffer_release(InstrBuffer* buffer) {
	if (buffer->inputs_buffer) {
		heap_release(buffer->inputs_buffer);
	}

	*buffer = (InstrBuffer) {};
}

InstrInputs instr_allocate_inputs_array(InstrBuffer* buffer, uint16_t count) {
	profile_scope_start(__func__);

	assert(buffer->inputs_buffer_capacity >= buffer->inputs_buffer_size);

	if (count == 0) {
		return (InstrInputs) { .start = UINT16_MAX, .count = 0 };
	}

	uint16_t free_size = buffer->inputs_buffer_capacity - buffer->inputs_buffer_size;
	if (count > free_size) {
		uint16_t new_capacity = max(32, buffer->inputs_buffer_capacity * 2);
		InstrIndex* new_buffer = heap_alloc_array(InstrIndex, new_capacity);

		if (buffer->inputs_buffer) {
			array_copy(new_buffer, buffer->inputs_buffer, buffer->inputs_buffer_size);
			heap_release(buffer->inputs_buffer);
		}

		asan_poison_memory_region(buffer->inputs_buffer, new_capacity - buffer->inputs_buffer_size);

		buffer->inputs_buffer = new_buffer;
		buffer->inputs_buffer_capacity = new_capacity;
	}

	InstrInputs inputs = {};
	inputs.start = buffer->inputs_buffer_size;
	inputs.count = count;

	buffer->inputs_buffer_size += count;
	assert(buffer->inputs_buffer_size <= buffer->inputs_buffer_capacity);

	profile_scope_end();
	return inputs;
}

//
// Instr
//

InstrIndex instr_new_int_const(InstrBuffer* buffer,
		Arena* allocator,
		uint64_t value,
		size_t int_size) {
	InstrIndex instr_index = instr_buffer_append(buffer, allocator);
	Instr* instr = instr_buffer_at(buffer, instr_index);

	switch (int_size) {
	case 1:
		assert(value <= 0xff);
		instr->kind = INSTR_CONST_8;
		instr->const_8.u = (uint8_t)value;
		break;
	case 2:
		assert(value <= 0xffff);
		instr->kind = INSTR_CONST_16;
		instr->const_16.u = (uint16_t)value;
		break;
	case 4:
		assert(value <= 0xffffffff);
		instr->kind = INSTR_CONST_32;
		instr->const_32.u = (uint32_t)value;
		break;
	case 8:
		assert(value <= 0xffffffffffffffff);
		instr->kind = INSTR_CONST_64;
		instr->const_64.u = value;
		break;
	default:
		unreachable();
	}

	return instr_index;
}

InstrIndex instr_new_jump(InstrBuffer* buffer,
		Arena* allocator,
		InstrIndex target,
		InstrIndex* io_state) {

	const Instr* io_state_instr = instr_buffer_at(buffer, *io_state);
	assert(io_state_instr->kind == INSTR_IO_STATE);

	InstrIndex i = instr_buffer_append(buffer, allocator);
	Instr* instr = instr_buffer_at(buffer, i);
	instr->kind = INSTR_JUMP;
	instr->jump.target_region = target;
	instr->jump.io_state = *io_state;

	*io_state = instr_new_io_state(buffer, allocator, INVALID_INSTR_INDEX);
	return i;
}

InstrIndex instr_new_return_value(InstrBuffer* buffer,
		Arena* allocator,
		InstrIndex value,
		InstrIndex* io_state) {

	const Instr* io_state_instr = instr_buffer_at(buffer, *io_state);
	assert(io_state_instr->kind == INSTR_IO_STATE);

	InstrIndex i = instr_buffer_append(buffer, allocator);
	Instr* instr = instr_buffer_at(buffer, i);
	instr->kind = INSTR_RETURN_VALUE;
	instr->return_value.value = value;
	instr->return_value.io_state = *io_state;
	return i;
}

InstrIndex instr_new_return(InstrBuffer* buffer, Arena* allocator, InstrIndex* io_state) {
	const Instr* io_state_instr = instr_buffer_at(buffer, *io_state);
	assert(io_state_instr->kind == INSTR_IO_STATE);

	InstrIndex i = instr_buffer_append(buffer, allocator);
	Instr* instr = instr_buffer_at(buffer, i);
	instr->kind = INSTR_RET;
	instr->ret.io_state = *io_state;
	return i;
}

InstrIndex instr_new_empty_phi(InstrBuffer* buffer, Arena* allocator) {
	InstrIndex i = instr_buffer_append(buffer, allocator);
	Instr* instr = instr_buffer_at(buffer, i);
	instr->kind = INSTR_PHI;
	return i;
}

InstrIndex instr_new_logical_shift_left_by(InstrBuffer* buffer,
		Arena* allocator,
		InstrIndex operand,
		uint8_t operand_size,
		uint8_t shift_count) {

	if (shift_count == 0) {
		return operand;
	}

	InstrIndex shift_count_const = instr_new_int_const(buffer,
			allocator,
			shift_count,
			1);

	InstrIndex shift_index = instr_buffer_append(buffer, allocator);
	Instr* shift_instr = instr_buffer_at(buffer, shift_index);
	shift_instr->bin_op.kind = INSTR_BIN_SHIFT_LEFT;
	shift_instr->bin_op.left = operand;
	shift_instr->bin_op.right = shift_count_const;

	switch (operand_size) {
	case 1:
		shift_instr->kind = INSTR_BIN_OP_8;
		break;
	case 2:
		shift_instr->kind = INSTR_BIN_OP_16;
		break;
	case 4:
		shift_instr->kind = INSTR_BIN_OP_32;
		break;
	case 8:
		shift_instr->kind = INSTR_BIN_OP_64;
		break;
	default:
		unreachable();
	}

	return shift_index;
}

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
	assert(has_flag(INSTR_FEATURES[last_instr_kind], INSTR_FEATURE_CONTROL));
	return true;
}

void instr_push_input_dependencies(const InstrBuffer* buffer,
		InstrInputs inputs,
		InstrQueue* out_dependencies) {

	for (uint16_t i = 0; i < inputs.count; i += 1) {
		instr_queue_push_back(out_dependencies, buffer->inputs_buffer[inputs.start + i]);
	}
}

//
// Live Ranges
//

inline bool _live_range_is_valid(const InstrLiveRange range) {
	return range.value != UINT32_MAX && range.start.value <= range.end.value;
}

inline bool _live_range_is_empty(const InstrLiveRange range) {
	return range.value == UINT32_MAX;
}

inline InstrLiveRange _live_range_merge(InstrLiveRange a, InstrLiveRange b) {
	InstrLiveRange out = {};
	out.start.value = min(a.start.value, b.start.value);
	out.end.value = max(a.end.value, b.end.value);
	return out;
}

// Returns a new range that includes the given instruction index
inline InstrLiveRange _live_range_extended(const InstrLiveRange range, InstrIndex instr_index) {
	InstrLiveRange new_range;
	new_range.start.value = min(range.start.value, instr_index.value);
	new_range.end.value = max(range.end.value, instr_index.value);
	return new_range;
}

InstrLiveRange* instr_compute_live_ranges(const InstrBuffer buffer,
		InstrIndex root_instr,
		Arena* allocator,
		Arena* temp_allocator) {
	profile_scope_start(__func__);

	ArenaRegion temp = arena_begin_temp(temp_allocator);

	InstrLiveRange* live_ranges = arena_alloc_array(allocator, InstrLiveRange, buffer.count);
	memset(live_ranges, 0xff, sizeof(*live_ranges) * buffer.count);

	InstrQueue stack;
	instr_queue_alloc(&stack, temp_allocator, buffer.count);

	BitArray visited_instr = bit_array_alloc(temp_allocator, buffer.count);
	bit_array_clear(&visited_instr);

	instr_queue_push_back(&stack, root_instr);

	while (stack.count) {
		InstrIndex instr_index = instr_queue_pop_back(&stack);
		if (instr_index.value == UINT16_MAX) {
			continue;
		}

		if (bit_array_get(&visited_instr, instr_index.value)) {
			// This instruction has already been visited
			continue;
		}

		bit_array_set(&visited_instr, instr_index.value, true);
		if (_live_range_is_valid(live_ranges[instr_index.value])) {
			live_ranges[instr_index.value] = _live_range_extended(
					live_ranges[instr_index.value],
					instr_index);
		} else if (_live_range_is_empty(live_ranges[instr_index.value])) {
			live_ranges[instr_index.value].start = instr_index;
			live_ranges[instr_index.value].end = instr_index;
		} else {
			panic("InstrLiveRange has invalid state");
		}

		size_t first_dep_index = stack.count;
		instr_enumerate_uses(&buffer, instr_index, &stack);

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
			InstrIndex dep_index = stack.buffer[i];
			if (dep_index.value >= buffer.count) {
				continue;
			}

			InstrLiveRange live_range = live_ranges[dep_index.value];
			if (_live_range_is_valid(live_range)) {
				live_range = _live_range_extended(live_range, instr_index);
			} else if (_live_range_is_empty(live_range)) {
				live_range.start = instr_index;
				live_range.end = instr_index;
			} else {
				panic("InstrLiveRange has invalid state");
			}

			live_ranges[dep_index.value] = live_range;
		}
	}

	for (size_t i = 0; i < buffer.count; i += 1) {
		if (live_ranges[i].value == UINT32_MAX) {
			continue;
		}

		const Instr* instr = &buffer.instr[i];
		if (instr->kind == INSTR_PHI) {
			InstrLiveRange phi_live_range = live_ranges[i];

			InstrInputs variants = instr->phi.variants;
			for (uint16_t j = variants.start; j < variants.start + variants.count; j += 1) {
				InstrIndex select_index = buffer.inputs_buffer[j];
				const Instr* select = &buffer.instr[select_index.value];
				assert(select->kind == INSTR_SELECT);

				const Instr* region = &buffer.instr[select->select.region.value];
				assert(region->kind == INSTR_REGION);

				InstrLiveRange* variant_live_range = &live_ranges[select->select.value.value];

				// NOTE: Extend the live range of the variant value to the end of the region, to
				//       make sure it stays alive until the end of the region.
				//
				//       And in case this region is part of a loop that looks like this:
				//       
				//       region A:
				//       1. phi
				//       2. <variant value computation>
				//       3. <other instructions>
				//       4. jump to A
				//
				//       we need to make sure that once the variant value is computed (at index 2), 
				//       it doesn't get override by other instructions (at index 3), until it
				//       reaches a jump back to the start of the region, where the phi node is
				//       placed.
				*variant_live_range = _live_range_extended(
						*variant_live_range,
						region->region.last_instr);

				phi_live_range = _live_range_merge(phi_live_range, *variant_live_range);
			}

			live_ranges[i] = phi_live_range;
		}
	}

	arena_end_temp(temp);
	profile_scope_end();
	return live_ranges;
}

InstrIndexArray _instr_gather_regions_in_dfs_order(const InstrBuffer instr_buffer,
		Arena* allocator,
		Arena* temp_allocator,
		InstrIndex start_region) {

	ArenaRegion temp = arena_begin_temp(temp_allocator);

	BitArray visited = bit_array_alloc(temp_allocator, instr_buffer.count);
	bit_array_clear(&visited);

	InstrQueue queue = {};
	instr_queue_alloc(&queue, temp_allocator, instr_buffer.count);
	instr_queue_push_back(&queue, start_region);

	InstrIndexArray dfs_order;
	dfs_order.instr = arena_alloc_array(allocator, InstrIndex, 0);
	dfs_order.count = 0;

	bit_array_set(&visited, start_region.value, true);

	while (queue.count > 0) {
		InstrIndex region_index = instr_queue_pop_back(&queue);
		const Instr* region = &instr_buffer.instr[region_index.value];
		assert(region->kind == INSTR_REGION);
		assert(bit_array_get(&visited, region_index.value));

		arena_alloc(allocator, InstrIndex);
		dfs_order.instr[dfs_order.count] = region_index;
		dfs_order.count += 1;

		assert(region->region.last_instr.value < instr_buffer.count);
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
		case INSTR_RET:
			break;
		default:
			unreachable();
		}
	}

	arena_end_temp(temp);
	return dfs_order;
}

String instr_format_input_instrs(const InstrIndex* input_instr_buffer,
		InstrInputs inputs,
		Arena* temp_allocator) {
	StringBuilder builder = { .arena = temp_allocator };

	str_builder_append_char(&builder, '[');
	for (uint16_t i = 0; i < inputs.count; i += 1) {
		InstrIndex input = input_instr_buffer[inputs.start + i];
		str_builder_append(&builder, STR_LIT("\033[33;1m%"));
		str_builder_append_int(&builder, input.value);
		str_builder_append(&builder, STR_LIT("\033[0m"));

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

void instr_replace_dead_instr(const InstrBuffer instr_buffer, const InstrLiveRange* live_ranges) {
	for (size_t i = 0; i < instr_buffer.count; i += 1) {
		if (live_ranges[i].value == UINT32_MAX) {
			instr_buffer.instr[i].kind = INSTR_NO_OP;
		}
	}
}
