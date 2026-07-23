#include "x64.h"

#include "code_gen/backends/x64_reg_alloc.h"

inline uint8_t _bit_count_from_index(uint8_t i) {
	return (1 << i) * 8;
}

static bool s_instr_storage_requiremenets_initialized = false;
X64InstrStorageRequirement s_instr_storage_requiremenets[INSTR_COUNT];

static void _init_storage_requiremenets() {
	if (s_instr_storage_requiremenets_initialized) {
		return;
	}

	X64InstrStorageRequirement* s = s_instr_storage_requiremenets;
	typedef X64InstrStorageRequirement T;
	s[INSTR_NO_OP]                  = (T) { .allowed_registers = 0, .reg_size = 0 };

	s[INSTR_UNINITIALIZED_8]        = (T) { .allowed_registers = UINT16_MAX, .reg_size = 8 };
	s[INSTR_UNINITIALIZED_16]       = (T) { .allowed_registers = UINT16_MAX, .reg_size = 16 };
	s[INSTR_UNINITIALIZED_32]       = (T) { .allowed_registers = UINT16_MAX, .reg_size = 32 };
	s[INSTR_UNINITIALIZED_64]       = (T) { .allowed_registers = UINT16_MAX, .reg_size = 64 };

	s[INSTR_CONST_8]                = (T) { .allowed_registers = UINT16_MAX, .reg_size = 8 };
	s[INSTR_CONST_16]               = (T) { .allowed_registers = UINT16_MAX, .reg_size = 16 };
	s[INSTR_CONST_32]               = (T) { .allowed_registers = UINT16_MAX, .reg_size = 32 };
	s[INSTR_CONST_64]               = (T) { .allowed_registers = UINT16_MAX, .reg_size = 64 };

	s[INSTR_CONST_STRING]           = (T) { .allowed_registers = UINT16_MAX, .reg_size = 64 };

	s[INSTR_BIN_OP_8]               = (T) { .allowed_registers = UINT16_MAX, .reg_size = 8 };
	s[INSTR_BIN_OP_16]              = (T) { .allowed_registers = UINT16_MAX, .reg_size = 16 };
	s[INSTR_BIN_OP_32]              = (T) { .allowed_registers = UINT16_MAX, .reg_size = 32 };
	s[INSTR_BIN_OP_64]              = (T) { .allowed_registers = UINT16_MAX, .reg_size = 64 };

	s[INSTR_BITWISE_NOT_8]          = (T) { .allowed_registers = UINT16_MAX, .reg_size = 8 };
	s[INSTR_BITWISE_NOT_16]         = (T) { .allowed_registers = UINT16_MAX, .reg_size = 8 };
	s[INSTR_BITWISE_NOT_32]         = (T) { .allowed_registers = UINT16_MAX, .reg_size = 8 };
	s[INSTR_BITWISE_NOT_64]         = (T) { .allowed_registers = UINT16_MAX, .reg_size = 8 };

	s[INSTR_BOOL_TO_INT]            = (T) { .allowed_registers = 0, .reg_size = 0 };

	s[INSTR_COMPARE_8]              = (T) { .allowed_registers = 0, .reg_size = 0 };
	s[INSTR_COMPARE_16]             = (T) { .allowed_registers = 0, .reg_size = 0 };
	s[INSTR_COMPARE_32]             = (T) { .allowed_registers = 0, .reg_size = 0 };
	s[INSTR_COMPARE_64]             = (T) { .allowed_registers = 0, .reg_size = 0 };

	s[INSTR_BOOL_TO_INT]            = (T) { .allowed_registers = UINT16_MAX, .reg_size = 8 };

	s[INSTR_NEGATE_8]               = (T) { .allowed_registers = UINT16_MAX, .reg_size = 8 };
	s[INSTR_NEGATE_16]              = (T) { .allowed_registers = UINT16_MAX, .reg_size = 8 };
	s[INSTR_NEGATE_32]              = (T) { .allowed_registers = UINT16_MAX, .reg_size = 8 };
	s[INSTR_NEGATE_64]              = (T) { .allowed_registers = UINT16_MAX, .reg_size = 8 };

	s[INSTR_CAST_TO_8]              = (T) { .allowed_registers = UINT16_MAX, .reg_size = 8 };
	s[INSTR_CAST_TO_16]             = (T) { .allowed_registers = UINT16_MAX, .reg_size = 16 };
	s[INSTR_CAST_TO_32]             = (T) { .allowed_registers = UINT16_MAX, .reg_size = 32 };
	s[INSTR_CAST_TO_64]             = (T) { .allowed_registers = UINT16_MAX, .reg_size = 64 };

	s[INSTR_PTR_LOAD_8]             = (T) { .allowed_registers = UINT16_MAX, .reg_size = 8 };
	s[INSTR_PTR_LOAD_16]            = (T) { .allowed_registers = UINT16_MAX, .reg_size = 16 };
	s[INSTR_PTR_LOAD_32]            = (T) { .allowed_registers = UINT16_MAX, .reg_size = 32 };
	s[INSTR_PTR_LOAD_64]            = (T) { .allowed_registers = UINT16_MAX, .reg_size = 64 };

	s[INSTR_PTR_STORE_8]            = (T) { .allowed_registers = 0, .reg_size = 8 };
	s[INSTR_PTR_STORE_16]           = (T) { .allowed_registers = 0, .reg_size = 16 };
	s[INSTR_PTR_STORE_32]           = (T) { .allowed_registers = 0, .reg_size = 32 };
	s[INSTR_PTR_STORE_64]           = (T) { .allowed_registers = 0, .reg_size = 64 };

	s[INSTR_LOAD_ARG_8]             = (T) { .allowed_registers = UINT16_MAX, .reg_size = 8 };
	s[INSTR_LOAD_ARG_16]            = (T) { .allowed_registers = UINT16_MAX, .reg_size = 16 };
	s[INSTR_LOAD_ARG_32]            = (T) { .allowed_registers = UINT16_MAX, .reg_size = 32 };
	s[INSTR_LOAD_ARG_64]            = (T) { .allowed_registers = UINT16_MAX, .reg_size = 64 };

	s[INSTR_BRANCH]                 = (T) { .allowed_registers = 0, .reg_size = 0 };
	s[INSTR_JUMP]                   = (T) { .allowed_registers = 0, .reg_size = 0 };

	s[INSTR_RETURN_VALUE]           = (T) { .allowed_registers = 0, .reg_size = 0 };

	s[INSTR_CALL_INDIRECT]          = (T) { .allowed_registers = UINT16_MAX, .reg_size = 64 };

	s[INSTR_REGION]                 = (T) { .allowed_registers = 0, .reg_size = 0 };

	s[INSTR_PHI]                    = (T) { .allowed_registers = UINT16_MAX, .reg_size = 64 };
	s[INSTR_SELECT]                 = (T) { .allowed_registers = 0, .reg_size = 0 };

	s_instr_storage_requiremenets_initialized = true;
};

static const char* X64_REG_BASE_NAMES[] = {
	"A",
	"C",
	"D",
	"B",
	"SP",
	"BP",
	"SI",
	"DI",
	"R8",
	"R9",
	"R10",
	"R11",
	"R12",
	"R13",
	"R14",
	"R15",
};

static void _format_reg_name(StringBuilder* builder, uint16_t reg_index, uint8_t reg_bit_count) {
	assert(reg_index < 16);

	char name_prefix = 0;
	char name_sufix = 0;

	switch (reg_bit_count) {
	case 8:
		name_sufix = 'L';
		break;
	case 16:
		if (reg_index < 4) {
			name_sufix = 'X';
		} else if (reg_index >= 8) {
			name_sufix = 'W';
		}
		break;
	case 32:
		if (reg_index < 4) {
			name_prefix = 'E';
			name_sufix = 'X';
		} else if (reg_index < 8) {
			name_prefix = 'E';
		} else {
			name_sufix = 'D';
		}
		
		break;
	case 64:
		if (reg_index < 4) {
			name_prefix = 'R';
			name_sufix = 'X';
		} else if (reg_index < 8) {
			name_prefix = 'R';
		}
		break;
	}

	if (name_prefix) {
		str_builder_append_char(builder, name_prefix);
	}

	str_builder_append_cstr(builder, X64_REG_BASE_NAMES[reg_index]);

	if (name_sufix) {
		str_builder_append_char(builder, name_sufix);
	}
}

//
// CFG Dominator Tree
//

typedef struct {
	uint16_t region_count;

	// Per region `BitArray` of regions that it dominates
	// The size of this array is equal to the total number of regions
	//
	// Size of each `BitArray` is also equal to the total number of regions
	BitArray* dominates;

	// An array of size eqaul to the total number of regions.
	// Maps region id to the immediate dominator of that region.
	//
	// `UINT16_MAX` means the regions doesn't have an immediate dominator.
	// Which is only true for the root region.
	uint16_t* immediate_dominators;
} CFGDominatorTree;

static bool _is_region_dominated_by(const CFGDominatorTree* tree,
		uint16_t dominated_region_id,
		uint16_t dominated_by_region_id) {
	const BitArray* dominated_regions = &tree->dominates[dominated_by_region_id];
	return bit_array_get(dominated_regions, dominated_region_id);
}

static CFGDominatorTree _build_cfg_dominator_tree(const InstrBuffer* instr_buffer,
		InstrIndex initial_region,
		Arena* allocator,
		Arena* temp_allocator) {
	profile_scope_start(__func__);
	ArenaRegion temp = arena_begin_temp(temp_allocator);

	BitArray visited_regions = bit_array_alloc(temp_allocator, instr_buffer->region_count);
	bit_array_clear(&visited_regions);

	InstrQueue stack;
	instr_queue_alloc(&stack, temp_allocator, instr_buffer->region_count);

	// Allocate the tree
	CFGDominatorTree tree;
	tree.region_count = instr_buffer->region_count;
	tree.dominates = arena_alloc_array(allocator, BitArray, tree.region_count);
	tree.immediate_dominators = arena_alloc_array(allocator, uint16_t, tree.region_count);

	for (uint16_t i = 0; i < instr_buffer->region_count; i += 1) {
		tree.dominates[i] = bit_array_alloc(allocator, tree.region_count);
		bit_array_clear(&tree.dominates[i]);
		bit_array_set(&tree.dominates[i], i, true);
	}

	// Push the initial region on the stack
	instr_queue_push_back(&stack, initial_region);

	{
		const Instr* initial = instr_buffer_at(instr_buffer, initial_region);
		bit_array_set(&visited_regions, initial->region.id, true);

		tree.immediate_dominators[initial->region.id] = UINT16_MAX;
	}
	
	// Build the tree
	while (stack.count) {
		InstrIndex region_instr_index = instr_queue_pop_back(&stack);
		const Instr* instr = instr_buffer_at(instr_buffer, region_instr_index);
		assert(instr->kind == INSTR_REGION);

		InstrIndex successors[2];
		size_t successor_count = 0;

		const Instr* last_instr = instr_buffer_at(instr_buffer, instr->region.last_instr);
		switch (last_instr->kind) {
		case INSTR_JUMP:
			successors[0] = last_instr->jump.target_region;
			successor_count = 1;
			break;
		case INSTR_BRANCH:
			successors[0] = last_instr->branch.true_region;
			successors[1] = last_instr->branch.false_region;
			successor_count = 2;
			break;
		case INSTR_RET:
		case INSTR_RETURN_VALUE:
			break;
		default:
			unreachable();
		}
		
		for (size_t i = 0; i < successor_count; i += 1) {
			InstrIndex successor_index = successors[i];
			const Instr* successor = instr_buffer_at(instr_buffer, successor_index);
			assert(successor->kind == INSTR_REGION);

			bool changed = false;
			if (bit_array_get(&visited_regions, successor->region.id)) {
				// Reset bit corresponding to the current region, so it doesn't interfere with the
				// `and` operation.
				bit_array_set(&tree.dominates[successor->region.id], successor->region.id, false);

				changed |= bit_array_and(&tree.dominates[instr->region.id],
						&tree.dominates[successor->region.id],
						&tree.dominates[successor->region.id]);

				bit_array_set(&tree.dominates[successor->region.id], successor->region.id, true);
			} else {
				changed |= bit_array_or(&tree.dominates[instr->region.id],
						&tree.dominates[successor->region.id],
						&tree.dominates[successor->region.id]);

				bit_array_set(&visited_regions, successor->region.id, true);
			}

			// If the set of regions dominated by the current one, was updated, we need to continue
			// the traversal and propagate the updates to the successors.
			if (changed) {
				instr_queue_push_back(&stack, successor_index);
			}
		}
	}

	// Now determine immediate dominators.
	for (uint16_t i = 0; i < instr_buffer->region_count; i += 1) {
		if (i == instr_region_id(instr_buffer, initial_region)) {
			continue;
		}

		BitArray* dominance = &tree.dominates[i];
		assert(bit_array_get(dominance, i));
		bit_array_set(dominance, i, false);

		bool found = false;
		for (uint16_t j = 0; j < instr_buffer->region_count; j += 1) {
			if (i == j) {
				continue;
			}

			if (bit_array_equal(dominance, &tree.dominates[j])) {
				tree.immediate_dominators[i] = j;
				found = true;
				bit_array_set(dominance, i, true);
				break;
			}
		}

		if (!found) {
			// This region is unreachable
			tree.immediate_dominators[i] = UINT16_MAX;
		}
	}

	// The initial region dones't have an immediate dominator
	// TODO: Need a better way to mark the initial region's dominator, since `UINT16_MAX` is also
	//       used for unreachable regions.
	tree.immediate_dominators[instr_region_id(instr_buffer, initial_region)] = UINT16_MAX;

	arena_end_temp(temp);

	profile_scope_end();
	return tree;
}

static void _print_dom_tree(const InstrBuffer* instr_buffer, CFGDominatorTree tree) {
	printf("dom tree:\n");
	for (uint16_t i = 0; i < instr_buffer->region_count; i += 1) {
		printf("region id=%u imm dom=%u: ",
				(uint32_t)i,
				(uint32_t)tree.immediate_dominators[i]);

		for (uint16_t j = 0; j < instr_buffer->region_count; j += 1) {
			if (bit_array_get(&tree.dominates[i], j)) {
				printf("%u ", (uint32_t)j);
			}
		}
		printf("\n");
	} 
}

// Finds a region where the control flow splits and later reaches both provided regions.
// The returned value is the region id.
static uint16_t _find_control_flow_split(const CFGDominatorTree* tree,
		uint16_t region_a_id,
		uint16_t region_b_id,
		Arena* temp_allocator) {
	profile_scope_start(__func__);
	ArenaRegion temp = arena_begin_temp(temp_allocator);
	BitArray visited_regions = bit_array_alloc(temp_allocator, tree->region_count);
	bit_array_clear(&visited_regions);

	// NOTE: There is no queue for `uint16_t`, so just reuse the implementation of `InstrIndex`
	InstrIndex backing_buffer[2];
	InstrQueue queue;
	instr_queue_init(&queue, backing_buffer, array_size(backing_buffer));

	instr_queue_push_back(&queue, (InstrIndex) { region_a_id });
	instr_queue_push_back(&queue, (InstrIndex) { region_b_id });

	while (queue.count) {
		InstrIndex region_id = instr_queue_pop_front(&queue);

		if (region_id.value == UINT16_MAX) {
			continue;
		}

		if (bit_array_get(&visited_regions, region_id.value)) {
			arena_end_temp(temp);
			profile_scope_end();
			return region_id.value;
		}

		bit_array_set(&visited_regions, region_id.value, true);
		instr_queue_push_back(&queue, (InstrIndex) { tree->immediate_dominators[region_id.value] });
	}

	unreachable();
	profile_scope_end();
	return UINT16_MAX;
}

//
// Code Generation
//

inline void _emit_load_const_64(CodeBuffer* buffer, X64Register reg, uint64_t value) {
	encode_2(buffer,
			MNEMONIC_MOV,
			operand_reg(reg, 64),
			operand_imm(value, 64));
}

inline void _emit_load_const_32(CodeBuffer* buffer, X64Register reg, uint32_t value) {
	encode_2(buffer,
			MNEMONIC_MOV,
			operand_reg(reg, 32),
			operand_imm(value, 32));
}

inline void _emit_load_const_16(CodeBuffer* buffer, X64Register reg, uint16_t value) {
	encode_2(buffer,
			MNEMONIC_MOV,
			operand_reg(reg, 16),
			operand_imm(value, 16));
}

inline void _emit_load_const_8(CodeBuffer* buffer, X64Register reg, uint8_t value) {
	encode_2(buffer,
			MNEMONIC_MOV,
			operand_reg(reg, 8),
			operand_imm(value, 8));
}

// NOTE: Not emitted when src and dst match. Don't use as a 32-bit movzx for that reason.
inline void _emit_mov_regs(CodeBuffer* buffer, X64Register src, X64Register dst, uint8_t reg_bit_count) {
	if (src == dst) {
		return;
	}

	encode_2(buffer,
			MNEMONIC_MOV,
			operand_reg(dst, reg_bit_count),
			operand_reg(src, reg_bit_count));
}

static void _emit_sub_rsp(CodeBuffer* buffer, uint32_t offset) {
	if (offset == 0) {
		return;
	}

	encode_2(buffer,
			MNEMONIC_SUB,
			operand_reg(X64_REG_SP, 64),
			operand_imm(offset, 32));
}

static void _emit_add_rsp(CodeBuffer* buffer, uint32_t offset) {
	if (offset == 0) {
		return;
	}

	encode_2(buffer,
			MNEMONIC_ADD,
			operand_reg(X64_REG_SP, 64),
			operand_imm(offset, 32));
}

static void _x64_generate_phi_copies(X64CodeGenerator* gen, uint16_t region_id, CodeBuffer* code_buffer) {
	profile_scope_start(__func__);
	const InstrIndexArray phi_variants = gen->phi_variants_per_region[region_id];
	const InstrIndex* phi_nodes = gen->phi_node_of_variant[region_id];

	for (uint16_t i = 0; i < phi_variants.count; i += 1) {
		InstrIndex variant_index = phi_variants.instr[i];
		InstrIndex phi_node_index = phi_nodes[i];

		const Instr* value = &gen->instr_buffer.instr[variant_index.value];
		const Instr* phi_node = &gen->instr_buffer.instr[phi_node_index.value];
		assert(phi_node->kind == INSTR_PHI);

		const InstrStorageLocation value_storage = gen->instr_storage[phi_variants.instr[i].value];
		const InstrStorageLocation phi_storage = gen->instr_storage[phi_node_index.value];

		if (phi_storage.kind == INSTR_STORAGE_NONE) {
			// Phi node was never allocated. It means it is also never going to be used.
			continue;
		}

		assert(value_storage.kind == INSTR_STORAGE_REG);
		assert(phi_storage.kind == INSTR_STORAGE_REG);

		uint8_t value_bit_size = s_instr_storage_requiremenets[value->kind].reg_size;
		_emit_mov_regs(code_buffer, value_storage.reg, phi_storage.reg, value_bit_size);
	}

	profile_scope_end();
}

typedef struct {
	X64Register* regs;
	size_t count;
} RegisterArray;

RegisterMoveArray _parallel_move_values(
		const InstrStorageLocation* input_instr_storage,
		const X64Register* expected_locs,
		size_t expected_loc_count,
		uint16_t allowed_temp_registers,
		Arena* allocator,
		Arena* temp_allocator) {
	profile_scope_start(__func__);

	// Validate that none of the expected and inputs locs overlap with temp registers
	//
	// In case we have a cycle, we need to save one of the registers to a temporary, same way when
	// we need to swap values of two variables:
	//
	// temp = a
	// a = b
	// b = temp
	//
	// However if the bit mask of allowed temporary registers contains the ones that are assigned to
	// `expected_locs` or `input_instr_storage`, saving to a temporary register might override one
	// of the values we're trying to parallel move into expected locations.
	for (size_t i = 0; i < expected_loc_count; i += 1) {
		assert_msg(!has_flag(allowed_temp_registers, 1 << expected_locs[i]),
				"Expected location overlaps with temporary registers");

		assert(input_instr_storage[i].kind == INSTR_STORAGE_REG);
		assert_msg(!has_flag(allowed_temp_registers, 1 << input_instr_storage[i].reg),
				"Expected location overlaps with temporary registers");
	}

	ArenaRegion temp = arena_begin_temp(temp_allocator);
	X64Register* map = arena_alloc_array(temp_allocator,
			X64Register,
			X64_REG_COUNT);

	const X64Register INVALID_REGISTER = -1;
	memset(map, 0xff, sizeof(*map) * X64_REG_COUNT);

	for (uint16_t i = 0; i < expected_loc_count; i += 1) {
		const InstrStorageLocation input_storage_loc = input_instr_storage[i];
		assert(input_storage_loc.kind == INSTR_STORAGE_REG);

		map[expected_locs[i]] = input_storage_loc.reg;
	}

	BitArray is_move_target = bit_array_alloc(temp_allocator, X64_REG_COUNT);
	bit_array_clear(&is_move_target);

	for (size_t i = 0; i < X64_REG_COUNT; i += 1) {
		if (map[i] != INVALID_REGISTER) {
			bit_array_set(&is_move_target, map[i], true);
		}
	}

	BitArray resolved_slots = bit_array_alloc(temp_allocator, X64_REG_COUNT);
	bit_array_clear(&resolved_slots);

	BitArray visited = bit_array_alloc(temp_allocator, X64_REG_COUNT);

	RegisterMoveArray result;
	result.moves = arena_alloc_array(allocator, RegisterMove, 0);
	result.count = 0;

	for (size_t reg_index = 0; reg_index < expected_loc_count; reg_index += 1) {
		X64Register reg = expected_locs[reg_index];
		if (bit_array_get(&resolved_slots, reg)) {
			continue;
		}

		if (bit_array_get(&is_move_target, reg)) {
			continue;
		}

		bit_array_clear(&visited);
		X64Register current_reg = reg;
		RegisterArray move_path;
		move_path.regs = arena_alloc_array(temp_allocator, X64Register, 0);
		move_path.count = 0;

		while (true) {
			assert_msg(bit_array_get(&visited, current_reg) == false, "Expected no cycles");
			assert(bit_array_get(&resolved_slots, current_reg) == false);

			bit_array_set(&visited, current_reg, true);
			bit_array_set(&resolved_slots, current_reg, true);

			arena_alloc(temp_allocator, X64Register);
			move_path.regs[move_path.count] = current_reg;
			move_path.count += 1;

			if (map[current_reg] == INVALID_REGISTER) {
				// No edge -> stop
				break;
			}

			current_reg = map[current_reg];
		}

		for (size_t i = 0; i + 1 < move_path.count; i += 1) {
			X64Register src = move_path.regs[i + 1];
			X64Register dst = move_path.regs[i];

			if (src == dst) {
				continue;
			}

			arena_alloc(allocator, RegisterMove);
			result.moves[result.count] = (RegisterMove) { .src = src, .dst = dst, };
			result.count += 1;
		}
	}

	for (size_t reg_index = 0; reg_index < expected_loc_count; reg_index += 1) {
		X64Register reg = expected_locs[reg_index];
		if (bit_array_get(&resolved_slots, reg)) {
			continue;
		}

		bit_array_clear(&visited);
		X64Register current_reg = reg;
		RegisterArray move_path;
		move_path.regs = arena_alloc_array(temp_allocator, X64Register, 0);
		move_path.count = 0;

		while (true) {
			if (bit_array_get(&visited, current_reg)) {
				// found a cycle -> stop
				break;
			}

			assert(bit_array_get(&resolved_slots, current_reg) == false);

			bit_array_set(&visited, current_reg, true);
			bit_array_set(&resolved_slots, current_reg, true);

			arena_alloc(temp_allocator, X64Register);
			move_path.regs[move_path.count] = current_reg;
			move_path.count += 1;

			if (map[current_reg] == INVALID_REGISTER) {
				// No edge -> stop
				break;
			}

			current_reg = map[current_reg];
		}

		if (move_path.count == 1) {
			// The input is already at the expected location
			continue;
		}

		assert_msg(allowed_temp_registers != 0, "Found a cycle, but there are no available temp"
				" registers to save one of the registers in the cycle");

		X64Register temp_save_register = count_trailing_zeros(allowed_temp_registers);

		arena_alloc(allocator, RegisterMove);
		result.moves[result.count] = (RegisterMove) {
			.src = move_path.regs[0],
			.dst = temp_save_register,
		};
		result.count += 1;

		for (size_t i = 0; i + 1 < move_path.count; i += 1) {
			X64Register src = move_path.regs[i + 1];
			X64Register dst = move_path.regs[i];

			if (src == dst) {
				continue;
			}

			arena_alloc(allocator, RegisterMove);
			result.moves[result.count] = (RegisterMove) { .src = src, .dst = dst, };
			result.count += 1;
		}

		arena_alloc(allocator, RegisterMove);
		result.moves[result.count] = (RegisterMove) {
			.src = temp_save_register,
			.dst = move_path.regs[move_path.count - 1],
		};
		result.count += 1;
	}

	for (size_t i = 0; i < expected_loc_count; i += 1) {
		assert(bit_array_get(&resolved_slots, expected_locs[i]));
	}

	arena_end_temp(temp);
	profile_scope_end();
	return result;
}

static void _arrange_phi_variants_for_size_computation(const InstrBuffer* instr_buffer,
		InstrIndex phi_index,
		BitArray* visited,
		InstrQueue* queue) {
	profile_scope_start(__func__);

	assert(!bit_array_get(visited, phi_index.value));

	bit_array_set(visited, phi_index.value, true);

	const Instr* phi = instr_buffer_at(instr_buffer, phi_index);

	InstrInputs inputs = phi->phi.variants;
	for (uint16_t i = 0; i < inputs.count; i += 1) {
		InstrIndex variant_index = instr_buffer->inputs_buffer[inputs.start + i];
		const Instr* variant = instr_buffer_at(instr_buffer, variant_index);
		const Instr* value = instr_buffer_at(instr_buffer, variant->select.value);

		if (value->kind != INSTR_PHI) {
			continue;
		}

		if (bit_array_get(visited, variant->select.value.value)) {
			continue;
		}

		_arrange_phi_variants_for_size_computation(instr_buffer,
				variant->select.value,
				visited,
				queue);
	}

	instr_queue_push_back(queue, phi_index);

	profile_scope_end();
}

static uint8_t* _arrange_phis_for_size_computation(const InstrBuffer* instr_buffer,
		Arena* allocator,
		Arena* temp_allocator) {

	profile_scope_start(__func__);

	ArenaRegion temp = arena_begin_temp(temp_allocator);
	BitArray visited = bit_array_alloc(temp_allocator, instr_buffer->count);
	bit_array_clear(&visited);

	InstrQueue queue;
	instr_queue_alloc(&queue, temp_allocator, instr_buffer->count);

	for (uint16_t i = 0; i < instr_buffer->count; i += 1) {
		if (instr_buffer->instr[i].kind != INSTR_PHI) {
			continue;
		}

		if (bit_array_get(&visited, i)) {
			continue;
		}

		_arrange_phi_variants_for_size_computation(instr_buffer,
				(InstrIndex) { i },
				&visited,
				&queue);
	}

	assert(queue.head == 0);

	uint8_t* sizes = arena_alloc_array_zeroed(allocator, uint8_t, instr_buffer->count);
	for (size_t i = 0; i < queue.count; i += 1) {
		InstrIndex phi_index = queue.buffer[i];
		assert(sizes[phi_index.value] == 0);

		const Instr* phi = instr_buffer_at(instr_buffer, phi_index);

		InstrInputs variants = phi->phi.variants;

		uint8_t phi_size = 0;
		for (uint16_t j = 0; j < variants.count; j += 1) {
			InstrIndex variant_index = instr_buffer->inputs_buffer[variants.start + j];
			const Instr* variant = instr_buffer_at(instr_buffer, variant_index);

			InstrIndex value_index = variant->select.value;
			if (value_index.value == phi_index.value) {
				continue;
			}

			const Instr* value = instr_buffer_at(instr_buffer, value_index);

			uint8_t variant_size = 0;
			if (value->kind == INSTR_PHI) {
				variant_size = sizes[value_index.value];
				if (variant_size == 0) {
					continue;
				}
			} else {
				variant_size = s_instr_storage_requiremenets[value->kind].reg_size;
			}

			assert(variant_size > 0);

			if (phi_size == 0) {
				phi_size = variant_size;
			} else {
				assert(phi_size == variant_size);
			}
		}

		sizes[phi_index.value] = phi_size;
	}

	arena_end_temp(temp);

	profile_scope_end();
	return sizes;
}

static uint8_t _get_instr_value_size(const X64CodeGenerator* gen, InstrIndex instr_index) {
	const InstrBuffer* instr_buffer = &gen->instr_buffer;
	const Instr* instr = instr_buffer_at(instr_buffer, instr_index);

	uint8_t value_size = 0;
	if (instr->kind == INSTR_PHI) {
		value_size = gen->phi_sizes[instr_index.value];
	} else {
		value_size = s_instr_storage_requiremenets[instr->kind].reg_size;
	}

	assert(value_size > 0);
	return value_size;
}

static void _emit_mul(CodeBuffer* buffer,
		MnemonicKind mnemonic,
		X64Register left_reg,
		X64Register right_reg,
		X64Register dst_reg,
		uint8_t bit_count) {
	// FIXME: Make sure that nothing else is using `AH` at this moment, since it will be
	//        overriden by the `imul` result.
	X64Register mul_output_reg = X64_REG_A;
	bool should_save_output = dst_reg != mul_output_reg;

	if (should_save_output) {
		encode_1(buffer, MNEMONIC_PUSH, operand_reg(mul_output_reg, 64));
	}

	if (left_reg == mul_output_reg) {
		encode_1(buffer, mnemonic, operand_reg(right_reg, bit_count));
	} else if (right_reg == mul_output_reg) {
		encode_1(buffer, mnemonic, operand_reg(left_reg, bit_count));
	} else {
		_emit_mov_regs(buffer, left_reg, mul_output_reg, bit_count);
		encode_1(buffer, mnemonic, operand_reg(right_reg, bit_count));
	}

	_emit_mov_regs(buffer, X64_REG_A, dst_reg, bit_count);

	if (should_save_output) {
		encode_1(buffer, MNEMONIC_POP, operand_reg(mul_output_reg, 64));
	}
}

static void _emit_div_mod(CodeBuffer* buffer,
		MnemonicKind mnemonic,
		X64Register left_reg,
		X64Register right_reg,
		X64Register dst_reg,
		bool select_quotient,
		uint8_t bit_count) {
	assert(mnemonic == MNEMONIC_DIV || mnemonic == MNEMONIC_IDIV);

	X64Register quotient_output_reg = X64_REG_A;
	X64Register remainder_output_reg = X64_REG_D;
	bool should_save_a = dst_reg != quotient_output_reg;
	bool should_save_d = dst_reg != remainder_output_reg;

	bool use_temp_r8 = left_reg == X64_REG_A || left_reg == X64_REG_D;
	bool use_temp_r9 = right_reg == X64_REG_A || right_reg == X64_REG_D;

	if (should_save_a) {
		encode_1(buffer, MNEMONIC_PUSH, operand_reg(quotient_output_reg, 64));
	}

	if (should_save_d) {
		encode_1(buffer, MNEMONIC_PUSH, operand_reg(remainder_output_reg, 64));
	}

	if (use_temp_r8) {
		encode_1(buffer, MNEMONIC_PUSH, operand_reg(X64_REG_8, 64));
		encode_2(buffer,
				MNEMONIC_MOV,
				operand_reg(X64_REG_8, bit_count),
				operand_reg(left_reg, bit_count));

		left_reg = X64_REG_8;
	}

	if (use_temp_r9) {
		encode_1(buffer, MNEMONIC_PUSH, operand_reg(X64_REG_9, 64));
		encode_2(buffer,
				MNEMONIC_MOV,
				operand_reg(X64_REG_9, bit_count),
				operand_reg(right_reg, bit_count));

		right_reg = X64_REG_9;
	}

	if (mnemonic == MNEMONIC_IDIV) {
		switch (bit_count) {
		case 8:
			encode_2(buffer, MNEMONIC_MOVSX, operand_reg(left_reg, 8), operand_reg(X64_REG_A, 16));
			break;
		case 16:
			_emit_mov_regs(buffer, left_reg, X64_REG_A, bit_count);
			encode_n(buffer, MNEMONIC_CWD, NULL, 0);
			break;
		case 32:
			_emit_mov_regs(buffer, left_reg, X64_REG_A, bit_count);
			encode_n(buffer, MNEMONIC_CDQ, NULL, 0);
			break;
		case 64:
			_emit_mov_regs(buffer, left_reg, X64_REG_A, bit_count);
			encode_n(buffer, MNEMONIC_CQO, NULL, 0);
			break;
		default:
			unreachable();
		}
	} else {
		switch (bit_count) {
		case 8:
			encode_2(buffer, MNEMONIC_MOVZX, operand_reg(left_reg, 8), operand_reg(X64_REG_A, 16));
			break;
		case 16:
			_emit_load_const_16(buffer, X64_REG_D, 0);
			_emit_mov_regs(buffer, left_reg, X64_REG_A, bit_count);
			break;
		case 32:
			_emit_load_const_32(buffer, X64_REG_D, 0);
			_emit_mov_regs(buffer, left_reg, X64_REG_A, bit_count);
			break;
		case 64:
			_emit_load_const_64(buffer, X64_REG_D, 0);
			_emit_mov_regs(buffer, left_reg, X64_REG_A, bit_count);
			break;
		default:
			unreachable();
		}
	}

	encode_1(buffer, mnemonic, operand_reg(right_reg, bit_count));

	if (bit_count == 8) {
		if (!select_quotient) {
			_emit_mov_regs(buffer, 4 /* AH */, X64_REG_A, 8);
		}

		_emit_mov_regs(buffer,
				select_quotient ? quotient_output_reg : X64_REG_A,
				dst_reg,
				bit_count);
	} else {
		_emit_mov_regs(buffer,
				select_quotient ? quotient_output_reg : remainder_output_reg,
				dst_reg,
				bit_count);
	}

	if (use_temp_r9) {
		encode_1(buffer, MNEMONIC_POP, operand_reg(X64_REG_9, 64));
	}

	if (use_temp_r8) {
		encode_1(buffer, MNEMONIC_POP, operand_reg(X64_REG_8, 64));
	}

	if (should_save_d) {
		encode_1(buffer, MNEMONIC_POP, operand_reg(remainder_output_reg, 64));
	}

	if (should_save_a) {
		encode_1(buffer, MNEMONIC_POP, operand_reg(quotient_output_reg, 64));
	}
}

// Well that is a mess!
void _emit_bitwise_shift(CodeBuffer* buffer,
		MnemonicKind mnemonic,
		X64Register value_reg,
		X64Register count_reg,
		X64Register dst_reg,
		uint8_t bit_count,
		Arena* allocator,
		Arena* temp_allocator) {

	bool dst_is_cl = dst_reg == X64_REG_C;
	bool should_save_rcx = value_reg != X64_REG_C && count_reg != X64_REG_C && dst_reg != X64_REG_C;

	if (should_save_rcx) {
		encode_1(buffer, MNEMONIC_PUSH, operand_reg(X64_REG_C, 64));
	}

	X64Register result_reg = dst_reg;
	if (dst_is_cl && value_reg != X64_REG_C) {
		encode_1(buffer, MNEMONIC_PUSH, operand_reg(value_reg, 64));
		result_reg = value_reg;
	} else if (dst_is_cl && count_reg != X64_REG_C) {
		encode_1(buffer, MNEMONIC_PUSH, operand_reg(count_reg, 64));
		result_reg = count_reg;
	}

	X64Register expected_locs[] = { result_reg, X64_REG_C };
	InstrStorageLocation input_locs[2];

	if (value_reg == X64_REG_C && count_reg == result_reg) {
		// Manually resolve the cycle.
		// Parallel moves won't do that, since it won't have any temp registers for that.
		encode_1(buffer, MNEMONIC_PUSH, operand_reg(value_reg, 64));
		encode_1(buffer, MNEMONIC_PUSH, operand_reg(count_reg, 64));

		encode_1(buffer, MNEMONIC_POP, operand_reg(value_reg, 64));
		encode_1(buffer, MNEMONIC_POP, operand_reg(count_reg, 64));

		input_locs[0] = (InstrStorageLocation) {
			.kind = INSTR_STORAGE_REG,
			.reg = count_reg,
		};
		input_locs[1] = (InstrStorageLocation) {
			.kind = INSTR_STORAGE_REG,
			.reg = value_reg,
		};
	} else {
		input_locs[0] = (InstrStorageLocation) {
			.kind = INSTR_STORAGE_REG,
			.reg = value_reg,
		};
		input_locs[1] = (InstrStorageLocation) {
			.kind = INSTR_STORAGE_REG,
			.reg = count_reg,
		};
	}

	RegisterMoveArray moves = _parallel_move_values(input_locs,
			expected_locs,
			2, 0,
			allocator,
			temp_allocator);

	for (size_t i = 0; i < moves.count; i += 1) {
		RegisterMove move = moves.moves[i];
		_emit_mov_regs(buffer, move.src, move.dst, 64);
	}

	encode_1(buffer, mnemonic, operand_reg(result_reg, bit_count));

	if (dst_is_cl) {
		encode_2(buffer,
				MNEMONIC_MOV,
				operand_reg(dst_reg, bit_count),
				operand_reg(result_reg, bit_count));
	}

	if (dst_is_cl && value_reg != X64_REG_C) {
		encode_1(buffer, MNEMONIC_POP, operand_reg(value_reg, 64));
	} else if (dst_is_cl && count_reg != X64_REG_C) {
		encode_1(buffer, MNEMONIC_POP, operand_reg(count_reg, 64));
	}

	if (should_save_rcx) {
		encode_1(buffer, MNEMONIC_POP, operand_reg(X64_REG_C, 64));
	}
}

static void _lower_instr(X64CodeGenerator* gen,
		InstrIndex instr_index,
		uint16_t region_id,
		CodeBuffer* buffer) {

	assert(instr_index.value < gen->instr_buffer.count);

	const InstrBuffer* instr_buffer = &gen->instr_buffer;
	const Instr* instr = &gen->instr_buffer.instr[instr_index.value];
	const InstrStorageLocation instr_storage = gen->instr_storage[instr_index.value];

	switch (instr->kind) {
	case INSTR_NO_OP:
		return;
	
	case INSTR_UNINITIALIZED_8:
	case INSTR_UNINITIALIZED_16:
	case INSTR_UNINITIALIZED_32:
	case INSTR_UNINITIALIZED_64:
		return;

	case INSTR_CONST_8:
		assert(instr_storage.kind == INSTR_STORAGE_REG);
		_emit_load_const_8(buffer, instr_storage.reg, instr->const_8.u);
		return;
	case INSTR_CONST_16:
		assert(instr_storage.kind == INSTR_STORAGE_REG);
		_emit_load_const_16(buffer, instr_storage.reg, instr->const_16.u);
		return;
	case INSTR_CONST_32:
		assert(instr_storage.kind == INSTR_STORAGE_REG);
		_emit_load_const_32(buffer, instr_storage.reg, instr->const_32.u);
		return;

	case INSTR_CONST_64:
		assert(instr_storage.kind == INSTR_STORAGE_REG);
		_emit_load_const_64(buffer, instr_storage.reg, instr->const_64.u);
		return;

	case INSTR_CONST_STRING: {
		assert(instr_storage.kind == INSTR_STORAGE_REG);

		uint32_t str_id = instr->const_string.string_id;
		const char* string = gen->merged_strings_buffer + gen->string_offsets[str_id];
		_emit_load_const_64(buffer, instr_storage.reg, (uint64_t)string);
		return;
	}

	case INSTR_BIN_OP_8:
	case INSTR_BIN_OP_16:
	case INSTR_BIN_OP_32:
	case INSTR_BIN_OP_64: {
		uint8_t bit_count = _bit_count_from_index(instr->kind - INSTR_BIN_OP_8);

		const InstrStorageLocation dst_loc = gen->instr_storage[instr_index.value];
		const InstrStorageLocation left_loc = gen->instr_storage[instr->bin_op.left.value];
		const InstrStorageLocation right_loc = gen->instr_storage[instr->bin_op.right.value];
		assert(dst_loc.kind == INSTR_STORAGE_REG);
		assert(left_loc.kind == INSTR_STORAGE_REG);
		assert(right_loc.kind == INSTR_STORAGE_REG);

		uint8_t left_reg;
		uint8_t right_reg;

		// NOTE: We have two input register and one output.
		//       It might happen that the output register overlaps with one of the input registers.
		//       When encoding the instruction the register on the left is the output one, this is
		//       fine if the output register of this bin op overlaps with left input. However the
		//       other might happen, and the output might overlap with the right input, in such case
		//       if the operation is commutative we can just swap the inputs.
		//       
		//       For non-commutative ops the solution for is:
		//       If output overlaps with the left input, encode as is, this case doesn't require
		//       any special treatment.
		//       
		//       Otherwise if the output overlaps with right right input, push the left register
		//       onto the stack, do the computation while storing the result in the left input
		//       register, move the result to the right register, and finally restore the right
		//       register.
		if (instr_bin_op_is_commutative(instr->bin_op.kind)) {
			if (right_loc.reg == dst_loc.reg) {
				_emit_mov_regs(buffer, right_loc.reg, dst_loc.reg, bit_count);

				left_reg = dst_loc.reg;
				right_reg = left_loc.reg;
			} else {
				_emit_mov_regs(buffer, left_loc.reg, dst_loc.reg, bit_count);

				left_reg = dst_loc.reg;
				right_reg = right_loc.reg;
			}
		} else {
			left_reg = left_loc.reg;
			right_reg = right_loc.reg;
		}

		switch (instr->bin_op.kind) {
		case INSTR_BIN_ADD:
			encode_2(buffer,
					MNEMONIC_ADD,
					operand_reg(left_reg, bit_count),
					operand_reg(right_reg, bit_count));
			return;
		case INSTR_BIN_SUB: {
			if (dst_loc.reg == right_loc.reg) {
				// NOTE: When saving the register, push/pop the whole 64-bit register
				encode_1(buffer,
						MNEMONIC_PUSH,
						operand_reg(left_loc.reg, 64));

				encode_2(buffer,
						MNEMONIC_SUB,
						operand_reg(left_reg, bit_count),
						operand_reg(right_reg, bit_count));

				_emit_mov_regs(buffer, left_loc.reg, right_loc.reg, bit_count);

				encode_1(buffer,
						MNEMONIC_POP,
						operand_reg(left_loc.reg, 64));
			} else if (dst_loc.reg == left_loc.reg) {
				encode_2(buffer,
						MNEMONIC_SUB,
						operand_reg(left_reg, bit_count),
						operand_reg(right_reg, bit_count));
			} else {
				encode_2(buffer,
						MNEMONIC_MOV,
						operand_reg(dst_loc.reg, bit_count),
						operand_reg(left_reg, bit_count));

				encode_2(buffer,
						MNEMONIC_SUB,
						operand_reg(dst_loc.reg, bit_count),
						operand_reg(right_reg, bit_count));
			}

			return;
		}
		case INSTR_BIN_IMUL:
			if (bit_count == 8) {
				_emit_mul(buffer, MNEMONIC_IMUL, left_reg, right_reg, dst_loc.reg, 8);
			} else {
				assert(bit_count == 16 || bit_count == 32 || bit_count == 64);

				encode_2(buffer,
						MNEMONIC_IMUL,
						operand_reg(left_reg, bit_count),
						operand_reg(right_reg, bit_count));
			}
			return;
		case INSTR_BIN_UMUL:
			_emit_mul(buffer, MNEMONIC_MUL, left_reg, right_reg, dst_loc.reg, bit_count);
			return;
		case INSTR_BIN_IDIV:
			_emit_div_mod(buffer, MNEMONIC_IDIV, left_reg, right_reg, dst_loc.reg, true, bit_count);
			return;
		case INSTR_BIN_UDIV:
			_emit_div_mod(buffer, MNEMONIC_DIV, left_reg, right_reg, dst_loc.reg, true, bit_count);
			return;
		case INSTR_BIN_IMOD:
			_emit_div_mod(buffer,
					MNEMONIC_IDIV,
					left_reg,
					right_reg,
					dst_loc.reg,
					false,
					bit_count);
			return;
		case INSTR_BIN_UMOD:
			_emit_div_mod(buffer,
					MNEMONIC_DIV,
					left_reg,
					right_reg,
					dst_loc.reg,
					false,
					bit_count);
			return;
		case INSTR_BIN_AND:
			encode_2(buffer,
					MNEMONIC_AND,
					operand_reg(left_reg, bit_count),
					operand_reg(right_reg, bit_count));
			return;
		case INSTR_BIN_OR:
			encode_2(buffer,
					MNEMONIC_OR,
					operand_reg(left_reg, bit_count),
					operand_reg(right_reg, bit_count));
			return;
		case INSTR_BIN_XOR:
			encode_2(buffer,
					MNEMONIC_XOR,
					operand_reg(left_reg, bit_count),
					operand_reg(right_reg, bit_count));
			return;
		case INSTR_BIN_SHIFT_LEFT:
			_emit_bitwise_shift(buffer,
					MNEMONIC_SHL,
					left_reg,
					right_reg,
					dst_loc.reg,
					bit_count,
					gen->allocator,
					gen->temp_allocator);
			return;
		case INSTR_BIN_SHIFT_RIGHT:
			_emit_bitwise_shift(buffer,
					MNEMONIC_SHR,
					left_reg,
					right_reg,
					dst_loc.reg,
					bit_count,
					gen->allocator,
					gen->temp_allocator);
			return;
		}

		unreachable();
	}

	case INSTR_PTR_LOAD_8:
	case INSTR_PTR_LOAD_16:
	case INSTR_PTR_LOAD_32:
	case INSTR_PTR_LOAD_64: {
		const InstrStorageLocation dst_loc = gen->instr_storage[instr_index.value];
		const InstrStorageLocation ptr_loc = gen->instr_storage[instr->ptr_load.ptr.value];
		assert(dst_loc.kind == INSTR_STORAGE_REG);
		assert(ptr_loc.kind == INSTR_STORAGE_REG);

		assert(_get_instr_value_size(gen, instr->ptr_load.ptr) == 64);

		uint8_t bit_count = _bit_count_from_index(instr->kind - INSTR_PTR_LOAD_8);
		encode_2(buffer,
				MNEMONIC_MOV,
				operand_reg(dst_loc.reg, bit_count),
				operand_mem(ptr_loc.reg, bit_count));
		return;
	}

	case INSTR_PTR_STORE_8:
	case INSTR_PTR_STORE_16:
	case INSTR_PTR_STORE_32:
	case INSTR_PTR_STORE_64: {
		const InstrStorageLocation ptr_loc = gen->instr_storage[instr->ptr_store.ptr.value];
		const InstrStorageLocation value_loc = gen->instr_storage[instr->ptr_store.value.value];
		assert(ptr_loc.kind == INSTR_STORAGE_REG);
		assert(value_loc.kind == INSTR_STORAGE_REG);

		assert(_get_instr_value_size(gen, instr->ptr_store.ptr) == 64);

		uint8_t bit_count = _bit_count_from_index(instr->kind - INSTR_PTR_STORE_8);
		encode_2(buffer,
				MNEMONIC_MOV,
				operand_mem(ptr_loc.reg, bit_count),
				operand_reg(value_loc.reg, bit_count));
		return;
	}

	case INSTR_LOAD_ARG_8:
	case INSTR_LOAD_ARG_16:
	case INSTR_LOAD_ARG_32:
	case INSTR_LOAD_ARG_64:
		// There is nothing to do. These instruction type is more of a hint
		// to where to look for the value, it doesn't get turned into any machine code.
		//
		// The register allocator allocates registers corresponding to the function
		// argument, and during the compilation of other instructions, the allocated
		// register is used as an input.
		return;

	case INSTR_BITWISE_NOT_8:
	case INSTR_BITWISE_NOT_16:
	case INSTR_BITWISE_NOT_32:
	case INSTR_BITWISE_NOT_64: {
		uint8_t bit_count = _bit_count_from_index(instr->kind - INSTR_BITWISE_NOT_8);

		const InstrStorageLocation dst_loc = gen->instr_storage[instr_index.value];
		const InstrStorageLocation operand_loc = gen->instr_storage[instr->negate.operand.value];

		assert(dst_loc.kind == INSTR_STORAGE_REG);
		assert(operand_loc.kind == INSTR_STORAGE_REG);

		_emit_mov_regs(buffer, operand_loc.reg, dst_loc.reg, bit_count);
		encode_1(buffer, MNEMONIC_NOT, operand_reg(dst_loc.reg, bit_count));
		return;
	}

	case INSTR_NOT:
		return;

	case INSTR_COMPARE_8:
	case INSTR_COMPARE_16:
	case INSTR_COMPARE_32:
	case INSTR_COMPARE_64: {
		const InstrStorageLocation left_loc = gen->instr_storage[instr->compare.left.value];
		const InstrStorageLocation right_loc = gen->instr_storage[instr->compare.right.value];
		assert(left_loc.kind == INSTR_STORAGE_REG);
		assert(right_loc.kind == INSTR_STORAGE_REG);

		uint8_t bit_count = _bit_count_from_index(instr->kind - INSTR_COMPARE_8);

		encode_2(buffer,
				MNEMONIC_CMP,
				operand_reg(left_loc.reg, bit_count),
				operand_reg(right_loc.reg, bit_count));
		return;
	}
	case INSTR_BOOL_TO_INT: {
		const InstrStorageLocation dst_loc = gen->instr_storage[instr_index.value];
		assert(dst_loc.kind == INSTR_STORAGE_REG);

		Instr* operand_instr = instr_buffer_at(instr_buffer, instr->bool_to_int.operand);
		assert(has_flag(INSTR_FEATURES[operand_instr->kind], INSTR_FEATURE_BOOL));
		
		// Consume all the `INSTR_NOT`
		bool condition_is_flipped = 0;
		while (operand_instr->kind == INSTR_NOT) {
			condition_is_flipped = !condition_is_flipped;
			operand_instr = instr_buffer_at(instr_buffer, operand_instr->not.operand);
		}

		InstrCompareKind compare_kind = operand_instr->compare.kind;
		if (condition_is_flipped) {
			compare_kind = instr_compare_kind_flip(compare_kind);
		}

		MnemonicKind mnemonic = 0;
		switch (compare_kind) {
		case INSTR_CMP_EQUAL:
			mnemonic = MNEMONIC_SETZ;
			break;
		case INSTR_CMP_NOT_EQUAL:
			mnemonic = MNEMONIC_SETNZ;
			break;
		case INSTR_CMP_LESS:
			mnemonic = MNEMONIC_SETL;
			break;
		case INSTR_CMP_LESS_OR_EQUAL:
			mnemonic = MNEMONIC_SETLE;
			break;
		case INSTR_CMP_GREATER:
			mnemonic = MNEMONIC_SETNLE;
			break;
		case INSTR_CMP_GREATER_OR_EQUAL:
			mnemonic = MNEMONIC_SETNL;
			break;
		}

		assert(mnemonic != 0);

		encode_1(buffer, mnemonic, operand_reg(dst_loc.reg, 8));
		return;							
	}
	
	case INSTR_NEGATE_8:
	case INSTR_NEGATE_16:
	case INSTR_NEGATE_32:
	case INSTR_NEGATE_64: {
		uint8_t bit_count = _bit_count_from_index(instr->kind - INSTR_NEGATE_8);

		const InstrStorageLocation dst_loc = gen->instr_storage[instr_index.value];
		const InstrStorageLocation operand_loc = gen->instr_storage[instr->negate.operand.value];

		assert(dst_loc.kind == INSTR_STORAGE_REG);
		assert(operand_loc.kind == INSTR_STORAGE_REG);

		_emit_mov_regs(buffer, operand_loc.reg, dst_loc.reg, bit_count);
		encode_1(buffer, MNEMONIC_NEG, operand_reg(dst_loc.reg, bit_count));
		return;
	}
	
	case INSTR_CAST_TO_8:
	case INSTR_CAST_TO_16:
	case INSTR_CAST_TO_32:
	case INSTR_CAST_TO_64: {
		const InstrStorageLocation dst_loc = gen->instr_storage[instr_index.value];
		const InstrStorageLocation src_loc = gen->instr_storage[instr->cast.value.value];

		InstrBuffer* instr_buffer = &gen->instr_buffer;
		Instr* value = instr_buffer_at(instr_buffer, instr->cast.value);

		uint8_t operand_size = _get_instr_value_size(gen, instr->cast.value);
		uint8_t output_size = 8 << (instr->kind - INSTR_CAST_TO_8);

		assert(dst_loc.kind == INSTR_STORAGE_REG);
		assert(src_loc.kind == INSTR_STORAGE_REG);

		if (operand_size == output_size) {
			_emit_mov_regs(buffer, src_loc.reg, dst_loc.reg, operand_size);
			return;
		}

		if (output_size < operand_size) {
			// NOTE: When casting to a smaller bit count, just copy the corresponding
			//       lower half of an input register
			encode_2(buffer,
					MNEMONIC_MOV,
					operand_reg(dst_loc.reg, output_size),
					operand_reg(src_loc.reg, output_size));
			return;
		}

		if (operand_size == 8 || operand_size == 16) {
			encode_2(buffer,
					MNEMONIC_MOVZX,
					operand_reg(src_loc.reg, operand_size),
					operand_reg(dst_loc.reg, output_size));
		} else if (operand_size == 32) {
			// NOTE: Moving (writing) to a 32-bit register zeros out the upper half of the
			//       corresponding 64-bit regiters.
			//       There is no `movzx` for zero extending 32-bit value to a 64-bit one.

			encode_2(buffer,
					MNEMONIC_MOV,
					operand_reg(dst_loc.reg, operand_size),
					operand_reg(src_loc.reg, operand_size));
		} else {
			panic("Not implemented for this operand size");
		}
		return;
	}

	case INSTR_BRANCH: {
		const Instr* condition_instr = instr_buffer_at(instr_buffer, instr->branch.condition);

		assert_msg(
				has_flag(INSTR_FEATURES[condition_instr->kind], INSTR_FEATURE_BOOL),
				"A condition instruction for `INSTR_BRANCH` must produce a boolean value");

		_x64_generate_phi_copies(gen, region_id, buffer);
		return;
	}
	case INSTR_JUMP: {
		_x64_generate_phi_copies(gen, region_id, buffer);
		return;
	}

	case INSTR_RETURN_VALUE: {
		InstrIndex return_value = instr->return_value.value;
		const InstrStorageLocation return_value_loc = gen->instr_storage[return_value.value];
		assert(return_value_loc.kind == INSTR_STORAGE_REG);

		_emit_mov_regs(buffer, return_value_loc.reg, X64_REG_A, 64);

		// Don't need to generate a `ret` instruction, since it is done later when the control
		// instructions at the end of each code block are generated
		return;
	}
	case INSTR_RET:
		// Don't need to generate a `ret` instruction, since it is done later when the control
		// instructions at the end of each code block are generated
		return;
	
	case INSTR_IO_STATE:
		return;
	
	case INSTR_CALL_INDIRECT: {
		assert(instr_storage.kind == INSTR_STORAGE_REG);

		const uint32_t SHADOW_SPACE_SIZE = 32;

		X64Register saved_registers[] = {
			X64_REG_A,
			X64_REG_C,
			X64_REG_D,
			X64_REG_8,
			X64_REG_9,
			X64_REG_10,
			X64_REG_11,
		};

		InstrInputs args = instr->call_indirect.args;

		// Push saved registers
		for (size_t i = 0; i < array_size(saved_registers); i += 1) {
			encode_1(buffer,
					MNEMONIC_PUSH,
					operand_reg(saved_registers[i], 64));
		}

		X64Register cdecl_arg_regs[] = { X64_REG_C, X64_REG_D, X64_REG_8, X64_REG_9 };

		assert(args.count <= array_size(cdecl_arg_regs));

		{
			uint16_t allowed_temp_registers = UINT16_MAX;
			allowed_temp_registers &= ~(1 << instr_storage.reg);

			for (uint16_t i = 0; i < args.count; i += 1) {
				InstrIndex arg_instr = gen->instr_buffer.inputs_buffer[args.start + i];
				InstrStorageLocation loc = gen->instr_storage[arg_instr.value];

				assert(loc.kind == INSTR_STORAGE_REG);
				allowed_temp_registers &= ~(1 << loc.reg);
			}

			for (size_t i = 0; i < gen->instr_with_storage_requirement.count; i += 1) {
				if (gen->instr_with_storage_requirement.instr[i].value != instr_index.value) {
					continue;
				}

				UInt16Array edges = gen->interference_graph[i];
				for (size_t j = 0; j < edges.count; j += 1) {
					InstrIndex interfering_instr = gen->instr_with_storage_requirement.instr[edges.values[j]];
					InstrStorageLocation loc = gen->instr_storage[interfering_instr.value];

					assert(loc.kind == INSTR_STORAGE_REG);
					allowed_temp_registers &= ~(1 << loc.reg);
				}
			}

			ArenaRegion temp = arena_begin_temp(gen->allocator);

			InstrStorageLocation input_instr_storage[array_size(cdecl_arg_regs)];
			for (uint16_t i = 0; i < args.count; i += 1) {
				InstrIndex arg_instr = gen->instr_buffer.inputs_buffer[args.start + i];
				input_instr_storage[i] = gen->instr_storage[arg_instr.value];
			}

			RegisterMoveArray parallel_moves = _parallel_move_values(
					input_instr_storage,
					cdecl_arg_regs,
					args.count,
					0,
					gen->allocator,
					gen->temp_allocator);

			for (size_t i = 0; i < parallel_moves.count; i += 1) {
				RegisterMove move = parallel_moves.moves[i];
				_emit_mov_regs(buffer, move.src, move.dst, 64);
			}

			arena_end_temp(temp);
		}

		uint16_t function_id = instr->call_indirect.function_index;
		assert(function_id < gen->ref_table->size);
		const FunctionRef* ref = &gen->ref_table->refs[function_id];

		_emit_load_const_64(buffer, X64_REG_A, (uint64_t)ref->address);

		// push shadow space
		_emit_sub_rsp(buffer, SHADOW_SPACE_SIZE);

		encode_1(buffer, MNEMONIC_CALL, operand_reg(X64_REG_A, 64));

		// pop shadow space
		_emit_add_rsp(buffer, SHADOW_SPACE_SIZE);

		// Now move the return value into a the proper register dedicated
		// exactly for the return value of this call instruction
		_emit_mov_regs(buffer, X64_REG_A, instr_storage.reg, 64);

		// Pop saved registers in reverse order
		for (size_t i = array_size(saved_registers); i > 0; i -= 1) {

			X64Register reg = saved_registers[i - 1];
			bool should_restore = instr_storage.reg != reg;
			if (should_restore) {
				encode_1(buffer,
						MNEMONIC_POP,
						operand_reg(reg, 64));
			} else {
				_emit_add_rsp(buffer, 8);
			}
		}

		return;
	}

	case INSTR_REGION:
		panic("`INSTR_REGION` are handled outside of this functions. If this `panic` has been"
				" reached, it means this function was accidentally called for a region");
	case INSTR_PHI:
		// Nothing to do here, everything is already handled during code gen of `INSTR_REGION`
		return;
	case INSTR_SELECT:
		return;
	case INSTR_COUNT:
		unreachable();
	}

	unreachable();
}

static size_t _compute_control_instr_encoding_size(const Instr* instr) {
	switch (instr->kind) {
	case INSTR_JUMP:
		return compute_encoding_size_1(MNEMONIC_JMP, operand_rel32(0));
	case INSTR_BRANCH:
		// A branch gets encoded as two jumps:
		// 1. Jump to the true region if condition is true
		// 2. Jump to the false region otherwise
		return compute_encoding_size_1(MNEMONIC_JZ, operand_rel32(0))
			+ compute_encoding_size_1(MNEMONIC_JMP, operand_rel32(0));
	case INSTR_RETURN_VALUE:
	case INSTR_RET:
		return 1;
	default:
		unreachable();
	}

	return 0;
}

static MnemonicKind _select_jmp_mnemonic(InstrCompareKind op) {
	switch (op) {
	case INSTR_CMP_EQUAL:
		return MNEMONIC_JZ;
	case INSTR_CMP_NOT_EQUAL:
		return MNEMONIC_JNZ;
	case INSTR_CMP_LESS:
		return MNEMONIC_JL;
	case INSTR_CMP_LESS_OR_EQUAL:
		return MNEMONIC_JLE;
	case INSTR_CMP_GREATER:
		return MNEMONIC_JNLE;
	case INSTR_CMP_GREATER_OR_EQUAL:
		return MNEMONIC_JNL;
	}

	unreachable();
	return 0;
}

static void _encode_control_instr(const Instr* instr,
		const InstrBuffer* instr_buffer,
		size_t current_block_end_offset,
		const size_t* code_block_offsets,
		CodeBuffer* buffer) {
	switch (instr->kind) {
	case INSTR_JUMP: {
		uint16_t target_region_id = instr_region_id(instr_buffer, instr->jump.target_region);
		size_t target_offset = code_block_offsets[target_region_id];
		assert(target_offset <= INT64_MAX);

		int64_t relative_offset = (int64_t)target_offset - ((int64_t)current_block_end_offset + 5);
		assert(relative_offset >= INT32_MIN);
		assert(relative_offset <= INT32_MAX);

		encode_1(buffer, MNEMONIC_JMP, operand_rel32((int32_t)relative_offset));
		break;
	}
	case INSTR_BRANCH: {
		// A branch gets encoded as two jumps:
		// 1. Jump to the true region if condition is true
		// 2. Jump to the false region otherwise

		MnemonicKind jump_to_true_mnemonic_kind = 0;

		const Instr* condition_instr = instr_buffer_at(instr_buffer, instr->branch.condition);

		bool condition_is_flipped = false;
		while (condition_instr->kind == INSTR_NOT) {
			condition_is_flipped = !condition_is_flipped;
			condition_instr = instr_buffer_at(instr_buffer, condition_instr->not.operand);
		}

		switch (condition_instr->kind) {
		case INSTR_COMPARE_8:
		case INSTR_COMPARE_16:
		case INSTR_COMPARE_32:
		case INSTR_COMPARE_64: 
			if (condition_is_flipped) {
				jump_to_true_mnemonic_kind = _select_jmp_mnemonic(
						instr_compare_kind_flip(condition_instr->compare.kind));
			} else {
				jump_to_true_mnemonic_kind = _select_jmp_mnemonic(condition_instr->compare.kind);
			}
			break;
		default:
			jump_to_true_mnemonic_kind = condition_is_flipped ? MNEMONIC_JNZ : MNEMONIC_JZ;
		}

		uint16_t true_region_id = instr_region_id(instr_buffer, instr->branch.true_region);
		size_t true_offset = code_block_offsets[true_region_id];
		assert(true_offset <= INT64_MAX);

		int64_t true_relative_offset = (int64_t)true_offset - ((int64_t)current_block_end_offset + 6);
		assert(true_relative_offset >= INT32_MIN);
		assert(true_relative_offset <= INT32_MAX);

		uint16_t false_region_id = instr_region_id(instr_buffer, instr->branch.false_region);
		size_t false_offset = code_block_offsets[false_region_id];
		assert(false_offset <= INT64_MAX);

		int64_t false_relative_offset = (int64_t)false_offset - ((int64_t)current_block_end_offset + 6 + 5);
		assert(false_relative_offset >= INT32_MIN);
		assert(false_relative_offset <= INT32_MAX);

		encode_1(buffer, jump_to_true_mnemonic_kind, operand_rel32((int32_t)true_relative_offset));
		encode_1(buffer, MNEMONIC_JMP, operand_rel32((int32_t)false_relative_offset));
		break;
	}
	case INSTR_RETURN_VALUE:
	case INSTR_RET:
		code_buffer_push_8(buffer, 0xc3); // ret
		break;
	default:
		unreachable();
	}

	return;
}

//
// Code Generation Stages
//

static void _run_reg_allocator(X64CodeGenerator* gen) {
	uint16_t allowed_registers = UINT16_MAX;
	allowed_registers &= ~(1 << X64_REG_SP);
	allowed_registers &= ~(1 << X64_REG_BP);

	// HACK: Some times the register allocator might allocate the whole register
	//       to some instruction and also it's high part to the other, thus any
	//       writes by any of the two instructions will be reflected in two places.
	allowed_registers &= ~(1 << X64_REG_SI);
	allowed_registers &= ~(1 << X64_REG_DI);

	RegisterAllocationResult result;
	result = x64_alloc_regs(&gen->instr_buffer,
			gen->live_ranges,
			allowed_registers,
			// Use `temp_allocator` as a persistent one, since register allocations are only used
			// within the backend
			gen->temp_allocator,
			gen->allocator);

	gen->instr_with_storage_requirement = result.instr_with_storage_requirement;
	gen->instr_storage = result.allocations;
	gen->interference_graph = result.interference_graph;

	if (has_flag(gen->flags, X64_PRINT_ASSIGNED_STORAGE_LOC)) {
		printf("Assigned storage locations:\n");
		for (size_t i = 0; i < gen->instr_buffer.count; i += 1) {
			ArenaRegion temp = arena_begin_temp(gen->temp_allocator);

			String storage_string = STR_LIT("none");

			if (gen->instr_storage[i].kind == INSTR_STORAGE_REG) {
				StringBuilder builder = { .arena = gen->temp_allocator };

				const InstrKind instr_kind = gen->instr_buffer.instr[i].kind;
				const X64InstrStorageRequirement storage_requirement =
					s_instr_storage_requiremenets[instr_kind];

				_format_reg_name(&builder,
						gen->instr_storage[i].reg,
						storage_requirement.reg_size);

				storage_string = builder.string;
			}

			printf("%zu: %.*s\n", i, STR_FMT(storage_string));

			arena_end_temp(temp);
		}
	}
}

static bool _x64_validate(X64CodeGenerator* gen) {
	profile_scope_start(__func__);

	bool result = true;

	// Validate symbols
	assert(gen->ref_table);

	const FunctionRefTable* ref_table = gen->ref_table;
	for (uint16_t i = 0; i < ref_table->size; i += 1) {
		const FunctionRef* ref = &ref_table->refs[i];

		if (ref->address == NULL) {
			printf("unresolved function symbol %.*s\n", STR_FMT(ref->name));
			result = false;
		}
	}

	profile_scope_end();
	return result;
}


static void _merge_string_consts(X64CodeGenerator* gen) {
	profile_scope_start(__func__);

	StringArray strings = gen->string_consts;

	gen->string_offsets = arena_alloc_array(gen->temp_allocator, size_t, strings.count);
	gen->merged_strings_buffer = arena_alloc_array(gen->allocator, char, 0);

	for (size_t i = 0; i < strings.count; i += 1) {
		size_t string_length = strings.values[i].length;

		// +1 for null-terminator
		char* string = arena_alloc_array(gen->allocator, char, string_length + 1);
		memcpy(string, strings.values[i].v, string_length);
		string[string_length] = 0;

		gen->string_offsets[i] = string - gen->merged_strings_buffer;
	}

	profile_scope_end();
}

//
// Instruction Scheduler
//
// Instruction scheduler is resposible for assigning each instruction to one of the regions in such
// a way, that whenever an instruction is about to execute, all of its inputs are guaranteed to be
// available.
//

static void _collect_phis(X64CodeGenerator* gen, Arena* allocator) {
	profile_scope_start(__func__);
	const InstrBuffer* instr_buffer = &gen->instr_buffer;

	uint16_t* phi_variant_counts_per_region = arena_alloc_array_zeroed(allocator,
			uint16_t,
			gen->instr_buffer.region_count);

	for (uint16_t i = 0; i < instr_buffer->count; i += 1) {
		const Instr* instr = &instr_buffer->instr[i];
		if (instr->kind != INSTR_PHI) {
			continue;
		}

		InstrInputs variants = instr->phi.variants;
		for (uint16_t j = 0; j < variants.count; j += 1) {
			InstrIndex select = instr_buffer->inputs_buffer[variants.start + j];
			const Instr* select_instr = instr_buffer_at(instr_buffer, select);

			uint16_t region_id = instr_region_id(instr_buffer, select_instr->select.region);
			phi_variant_counts_per_region[region_id] += 1;

		}
	}

	InstrIndexArray* phi_variants_per_region = arena_alloc_array_zeroed(allocator,
			InstrIndexArray,
			instr_buffer->region_count);

	InstrIndex** phi_node_of_variant = arena_alloc_array(allocator,
			InstrIndex*,
			instr_buffer->region_count);

	for (uint16_t i = 0; i < gen->instr_buffer.region_count; i += 1) {
		phi_variants_per_region[i].instr = arena_alloc_array(allocator,
				InstrIndex,
				phi_variant_counts_per_region[i]);
	}

	for (uint16_t i = 0; i < gen->instr_buffer.region_count; i += 1) {
		phi_node_of_variant[i] = arena_alloc_array(allocator,
				InstrIndex,
				phi_variant_counts_per_region[i]);
	}

	// Sort all the phi variants into the arrays corresponding to the region where the variant must
	// be placed.
	for (uint16_t i = 0; i < instr_buffer->count; i += 1) {
		const Instr* instr = &instr_buffer->instr[i];
		if (instr->kind != INSTR_PHI) {
			continue;
		}

		InstrInputs variants = instr->phi.variants;
		for (uint16_t j = 0; j < variants.count; j += 1) {
			InstrIndex select = instr_buffer->inputs_buffer[variants.start + j];
			const Instr* select_instr = instr_buffer_at(instr_buffer, select);

			uint16_t region_id = instr_region_id(instr_buffer, select_instr->select.region);
			InstrIndexArray* variants = &phi_variants_per_region[region_id];

			variants->instr[variants->count] = select_instr->select.value;
			phi_node_of_variant[region_id][variants->count] = (InstrIndex) { i };
			variants->count += 1;
		}
	}

	if (has_flag(gen->flags, X64_DEBUG_LOG)) {
		for (uint16_t region_id = 0; region_id < instr_buffer->region_count; region_id += 1) {
			uint16_t phi_count = phi_variant_counts_per_region[region_id];
			printf("%u -> %u:\n", (uint32_t)region_id, (uint32_t)phi_count);

			for (uint16_t i = 0; i < phi_count; i += 1) {
				printf("phi: %u variant: %u\n",
						(uint32_t)phi_node_of_variant[region_id][i].value,
						(uint32_t)phi_variants_per_region[region_id].instr[i].value);
			}
		}
	}

	gen->phi_variant_counts_per_region = phi_variant_counts_per_region;
	gen->phi_variants_per_region = phi_variants_per_region;
	gen->phi_node_of_variant = phi_node_of_variant;

	profile_scope_end();
}

static bool _validate_instr_scheduling_for_region(const InstrBuffer* instr_buffer,
		uint16_t* instr_scheduled_region, 
		uint16_t* instr_position_in_region,
		uint16_t current_region_id,
		InstrIndexArray scheduled,
		const CFGDominatorTree* dom_tree,
		Arena* temp_allocator) {
	profile_scope_start(__func__);

	bool valid = true;

	for (size_t i = 0; i < scheduled.count; i += 1) {
		InstrIndex current_instr = scheduled.instr[i];
		const Instr* instr = instr_buffer_at(instr_buffer, scheduled.instr[i]);

		if (instr->kind == INSTR_SELECT) {
			uint16_t region_assigned_to_input = instr_scheduled_region[instr->select.value.value];
			uint16_t expected_region = instr_region_id(instr_buffer, instr->select.region);

			bool input_is_available = _is_region_dominated_by(dom_tree,
					region_assigned_to_input,
					expected_region);

			if (!input_is_available) {
				debug_log_error(
						"Value definition '%u' appears before its input '%u'. "
						"Input is not available in region with id '%u', since it is placed in"
						" region with id '%u'",
						(uint32_t)scheduled.instr[i].value,
						(uint32_t)instr->select.value.value,
						expected_region,
						region_assigned_to_input);

				valid = false;
			}
		} else {
			ArenaRegion inner_temp = arena_begin_temp(temp_allocator);

			InstrQueue queue;
			instr_queue_alloc(&queue, temp_allocator, instr_buffer->count);

			instr_enumerate_uses(instr_buffer, scheduled.instr[i], &queue);

			for (size_t j = 0; j < queue.count; j += 1) {
				InstrIndex input = queue.buffer[j];
				if (input.value == INVALID_INSTR_INDEX.value) {
					continue;
				}

				const Instr* input_instr = instr_buffer_at(instr_buffer, input);
				if (input_instr->kind == INSTR_REGION) {
					continue;
				}

				bool input_is_available = true;
				if (instr_scheduled_region[input.value] == current_region_id) {
					// The input and the current instruction are in the same region.
					// 
					// Check whether the input is placed before the def (current instruction).

					uint16_t current_instr_position = instr_position_in_region[current_instr.value];
					uint16_t input_instr_position = instr_position_in_region[input.value];

					input_is_available = input_instr_position < current_instr_position;
				} else {
					// Input and def are in different regions.
					//
					// The input is only then available, when the input's region is dominated by
					// current instruction's region.
					input_is_available = _is_region_dominated_by(dom_tree,
							instr_scheduled_region[input.value],
							current_region_id);

				}

				if (input_is_available) {
					continue;
				}

				debug_log_error(
						"Input '%u' for value definition '%u' is not available on this "
						"control path\n"
						"\n"
						"Def '%u' is scheduled to execute in region with id '%u' at index '%u'\n"
						"Input '%u' is scheduled to execute in region with id '%u' at index '%u'\n",
						(uint32_t)input.value,
						(uint32_t)scheduled.instr[i].value,

						(uint32_t)scheduled.instr[i].value,
						(uint32_t)current_region_id,
						(uint32_t)instr_position_in_region[scheduled.instr[i].value],

						(uint32_t)input.value,
						(uint32_t)instr_scheduled_region[input.value],
						(uint32_t)instr_position_in_region[input.value]);

				valid = false;
			}

			arena_end_temp(inner_temp);
		}
	}
	
	profile_scope_end();
	return valid;
}

typedef struct {
	uint16_t decided_region_id;
	uint16_t order_in_region;
} InstrSchedulingState;

typedef struct {
	const InstrBuffer* instr_buffer;
	InstrSchedulingState* states;
	const CFGDominatorTree* dom_tree;

	Arena* temp_allocator;

	// For each region stores a monotonically increasing integer.
	// This integer tells the order in which each instruction was added to this region.
	uint16_t* next_position_in_region;

	uint16_t current_region_id;
	uint16_t current_order;
	InstrIndex def;
} InstrSchedulingContext;

// Checks whether the `input_instr_index` is guaranteed to be available in the `region_id`.
//
// If it's not, uplifts `input_instr_index`. And pushes it onto the queue, since now
// `input_instr_index` and it's dependencies need to be rescheduled.
static void _try_enqueue_for_scheduling(InstrQueue* queue,
		InstrSchedulingContext* context,
		uint16_t region_id,
		InstrIndex input_instr_index) {

	uint16_t input_instr_region_id = context->states[input_instr_index.value].decided_region_id;
	if (input_instr_region_id == UINT16_MAX) {
		uint16_t order_in_region = context->next_position_in_region[region_id];
		context->next_position_in_region[region_id] += 1;

		context->states[input_instr_index.value].decided_region_id = region_id;
		context->states[input_instr_index.value].order_in_region = order_in_region;
		instr_queue_push_back(queue, input_instr_index);
		return;
	}

	if (input_instr_region_id == context->current_region_id) {
		if (context->states[input_instr_index.value].order_in_region < context->current_order) {
			uint16_t order_in_region = context->next_position_in_region[region_id];
			context->next_position_in_region[region_id] += 1;

			context->states[input_instr_index.value].decided_region_id = region_id;
			context->states[input_instr_index.value].order_in_region = order_in_region;
			instr_queue_push_back(queue, input_instr_index);

			return;
		}
	}

	bool is_input_dominated = _is_region_dominated_by(context->dom_tree,
				input_instr_region_id,
				region_id);

	// The input is guaranteed to appear before the instr at `instr_index`
	if (is_input_dominated) {
		return;
	}

	// The input is not guaranteed to appear before the def, so we need to uplift it to the common
	// region.
	
	uint16_t common_region_id = _find_control_flow_split(context->dom_tree,
			context->states[input_instr_index.value].decided_region_id,
			region_id,
			context->temp_allocator);

	uint16_t order_in_region = context->next_position_in_region[common_region_id];
	context->next_position_in_region[common_region_id] += 1;

	context->states[input_instr_index.value].decided_region_id = common_region_id;
	context->states[input_instr_index.value].order_in_region = order_in_region;

	instr_queue_push_back(queue, input_instr_index);
}

static void _enqueue_inputs_for_scheduling(InstrQueue* queue,
		InstrIndex instr_index,
		InstrSchedulingContext* context) {
	profile_scope_start(__func__);
	const InstrBuffer* instr_buffer = context->instr_buffer;
	const Instr* instr = instr_buffer_at(instr_buffer, instr_index);

	uint16_t current_region_id = context->states[instr_index.value].decided_region_id;
	assert_msg(current_region_id != UINT16_MAX,
			"Instr at `instr_index` must have an already assigned region id");

	context->current_region_id = current_region_id;
	context->current_order = context->states[instr_index.value].order_in_region;
	context->def = instr_index;

	switch (instr->kind) {
	case INSTR_NO_OP:
	case INSTR_UNINITIALIZED_8:
	case INSTR_UNINITIALIZED_16:
	case INSTR_UNINITIALIZED_32:
	case INSTR_UNINITIALIZED_64:
	case INSTR_CONST_8:
	case INSTR_CONST_16:
	case INSTR_CONST_32:
	case INSTR_CONST_64:
	case INSTR_CONST_STRING:
		break;
	case INSTR_BIN_OP_8:
	case INSTR_BIN_OP_16:
	case INSTR_BIN_OP_32:
	case INSTR_BIN_OP_64:
		_try_enqueue_for_scheduling(queue, context, current_region_id, instr->bin_op.left);
		_try_enqueue_for_scheduling(queue, context, current_region_id, instr->bin_op.right);
		break;
	case INSTR_PTR_LOAD_8:
	case INSTR_PTR_LOAD_16:
	case INSTR_PTR_LOAD_32:
	case INSTR_PTR_LOAD_64:
		_try_enqueue_for_scheduling(queue, context, current_region_id, instr->ptr_load.io_state);
		_try_enqueue_for_scheduling(queue, context, current_region_id, instr->ptr_load.ptr);
		break;
	case INSTR_PTR_STORE_8:
	case INSTR_PTR_STORE_16:
	case INSTR_PTR_STORE_32:
	case INSTR_PTR_STORE_64:
		_try_enqueue_for_scheduling(queue, context, current_region_id, instr->ptr_store.io_state);
		_try_enqueue_for_scheduling(queue, context, current_region_id, instr->ptr_store.ptr);
		_try_enqueue_for_scheduling(queue, context, current_region_id, instr->ptr_store.value);
		break;
	case INSTR_LOAD_ARG_8:
	case INSTR_LOAD_ARG_16:
	case INSTR_LOAD_ARG_32:
	case INSTR_LOAD_ARG_64:
		break;
	case INSTR_NOT:
		_try_enqueue_for_scheduling(queue, context, current_region_id, instr->not.operand);
		break;
	case INSTR_COMPARE_8:
	case INSTR_COMPARE_16:
	case INSTR_COMPARE_32:
	case INSTR_COMPARE_64:
		_try_enqueue_for_scheduling(queue, context, current_region_id, instr->compare.left);
		_try_enqueue_for_scheduling(queue, context, current_region_id, instr->compare.right);
		break;
	case INSTR_BOOL_TO_INT:
		_try_enqueue_for_scheduling(queue, context, current_region_id, instr->bool_to_int.operand);
		break;
	case INSTR_NEGATE_8:
	case INSTR_NEGATE_16:
	case INSTR_NEGATE_32:
	case INSTR_NEGATE_64:
		_try_enqueue_for_scheduling(queue, context, current_region_id, instr->negate.operand);
		break;
	case INSTR_BITWISE_NOT_8:
	case INSTR_BITWISE_NOT_16:
	case INSTR_BITWISE_NOT_32:
	case INSTR_BITWISE_NOT_64:
		_try_enqueue_for_scheduling(queue, context, current_region_id, instr->bitwise_not.operand);
		break;
	case INSTR_CAST_TO_8:
	case INSTR_CAST_TO_16:
	case INSTR_CAST_TO_32:
	case INSTR_CAST_TO_64:
		_try_enqueue_for_scheduling(queue, context, current_region_id, instr->cast.value);
		break;
	case INSTR_BRANCH:
		_try_enqueue_for_scheduling(queue, context, current_region_id, instr->branch.io_state);
		_try_enqueue_for_scheduling(queue, context, current_region_id, instr->branch.condition);
		break;
	case INSTR_JUMP:
		_try_enqueue_for_scheduling(queue, context, current_region_id, instr->jump.io_state);
		break;
	case INSTR_RETURN_VALUE:
		_try_enqueue_for_scheduling(queue, context, current_region_id, instr->return_value.io_state);
		_try_enqueue_for_scheduling(queue, context, current_region_id, instr->return_value.value);
		break;
	case INSTR_RET:
		_try_enqueue_for_scheduling(queue, context, current_region_id, instr->ret.io_state);
		break;
	case INSTR_IO_STATE:
		if (instr->io_state.producer.value != INVALID_INSTR_INDEX.value) {
			_try_enqueue_for_scheduling(queue, context, current_region_id, instr->io_state.producer);
		}

		break;
	case INSTR_CALL_INDIRECT: {
		_try_enqueue_for_scheduling(queue, context, current_region_id, instr->call_indirect.io_state);

		InstrInputs args = instr->call_indirect.args;
		for (uint16_t i = 0; i < args.count; i += 1) {
			InstrIndex arg_instr = instr_buffer->inputs_buffer[args.start + i];
			_try_enqueue_for_scheduling(queue, context, current_region_id, arg_instr);
		}

		break;
	}
	case INSTR_REGION:
		unreachable();
	case INSTR_PHI: {
		InstrInputs variants = instr->phi.variants;
		for (uint16_t i = 0; i < variants.count; i += 1) {
			InstrIndex variant = instr_buffer->inputs_buffer[variants.start + i];
			_try_enqueue_for_scheduling(queue, context, current_region_id, variant);
		}
		break;
	}
	case INSTR_SELECT: {
		uint16_t region_id = instr_region_id(instr_buffer, instr->select.region);
		// NOTE: Here we expect the value to be available at ehd END of the region with id
		//       `region_id`
		context->current_region_id = region_id;
		context->current_order = 0;
		_try_enqueue_for_scheduling(queue, context, region_id, instr->select.value);
		break;
	}
	case INSTR_COUNT:
		unreachable();
	}

	profile_scope_end();
}

// Sorts instructions in descending order based on `order_in_region` stored in
// `InstrSchedulingState`
static void _sort_scheduled_instr(InstrIndexArray instr_array, const InstrSchedulingState* states) {
	for (size_t i = 0; i < instr_array.count; i += 1) {
		for (size_t j = 1; j < instr_array.count; j += 1) {
			InstrIndex instr_a = instr_array.instr[j - 1];
			InstrIndex instr_b = instr_array.instr[j];

			uint16_t order_a = states[instr_a.value].order_in_region;
			uint16_t order_b = states[instr_b.value].order_in_region;

			if (order_a < order_b) {
				instr_array.instr[j] = instr_a;
				instr_array.instr[j - 1] = instr_b;
			}
		}
	}
}

typedef struct {
	// For each instruction in `instr_buffer` stores the `region_id` of the region, where this
	// instruction is scheduled.
	uint16_t* instr_scheduled_region;

	// Accompanies the above array providing the index of that instruction in the scheduled region.
	uint16_t* instr_position_in_region;

	// An array that for each region stores an array of instructions that belong to it.
	InstrIndexArray* scheduled_instr;
} SchedulingResult;

static void _schedule_instr(const InstrBuffer* instr_buffer,
		InstrIndexArray scheduled_regions,
		const CFGDominatorTree* dom_tree,
		SchedulingResult* out_result,
		Arena* allocator,
		Arena* temp_allocator) {
	profile_scope_start(__func__);
	ArenaRegion temp = arena_begin_temp(temp_allocator);

	InstrSchedulingState* states = arena_alloc_array(temp_allocator,
			InstrSchedulingState,
			instr_buffer->count);
	memset(states, 0xff, sizeof(*states) * instr_buffer->count);

	uint16_t* next_position_in_region = arena_alloc_array_zeroed(temp_allocator,
			uint16_t,
			instr_buffer->region_count);

	InstrSchedulingContext context;
	context.instr_buffer = instr_buffer;
	context.states = states;
	context.dom_tree = dom_tree;
	context.temp_allocator = temp_allocator;
	context.next_position_in_region = next_position_in_region;

	{
		for (size_t i = 0; i < scheduled_regions.count; i += 1) {
			InstrIndex root_region_index = scheduled_regions.instr[i];
			const Instr* root_region = instr_buffer_at(instr_buffer, root_region_index);

			uint16_t current_region_id = root_region->region.id;

			uint16_t order_in_region = next_position_in_region[current_region_id];
			next_position_in_region[current_region_id] += 1;

			states[root_region->region.last_instr.value].decided_region_id = current_region_id;
			states[root_region->region.last_instr.value].order_in_region = order_in_region;
		}
	}

	{
		ArenaRegion temp1 = arena_begin_temp(temp_allocator);

		InstrQueue queue;
		instr_queue_alloc(&queue, temp_allocator, instr_buffer->count);

		for (size_t i = 0; i < scheduled_regions.count; i += 1) {
			InstrIndex root_region_index = scheduled_regions.instr[i];
			uint16_t current_region_id = instr_region_id(instr_buffer, root_region_index);

			const Instr* root_region = instr_buffer_at(instr_buffer, root_region_index);
			instr_queue_push_back(&queue, root_region->region.last_instr);

			while (queue.count) {
				InstrIndex instr_index = instr_queue_pop_front(&queue);

				InstrSchedulingState* instr_state = &states[instr_index.value];
				assert(instr_state->decided_region_id < instr_buffer->region_count);

				_enqueue_inputs_for_scheduling(&queue, instr_index, &context);
			}
		}

		arena_end_temp(temp1);
	}

	// Prepare all the necessary buffers to store the scheduling results.
	uint16_t* instr_count_per_region = arena_alloc_array_zeroed(temp_allocator,
			uint16_t,
			instr_buffer->region_count);

	for (uint16_t i = 0; i < instr_buffer->count; i += 1) {
		if (states[i].decided_region_id == UINT16_MAX) {
			// This instruction doesn't belong to any of the regions.
			continue;
		}

		instr_count_per_region[states[i].decided_region_id] += 1;
	}

	InstrIndexArray* scheduled_instr_per_region = arena_alloc_array(allocator,
			InstrIndexArray,
			instr_buffer->region_count);

	for (uint16_t i = 0; i < instr_buffer->region_count; i += 1) {
		scheduled_instr_per_region[i].instr = arena_alloc_array(allocator,
				InstrIndex,
				instr_count_per_region[i]);
		scheduled_instr_per_region[i].count = 0;
	}

	uint16_t* instr_scheduled_region = arena_alloc_array(allocator,
			uint16_t,
			instr_buffer->count);
	uint16_t* instr_position_in_region = arena_alloc_array(allocator,
			uint16_t,
			instr_buffer->count);

	memset(instr_scheduled_region, 0xff, sizeof(*instr_scheduled_region) * instr_buffer->count);
	memset(instr_position_in_region, 0xff, sizeof(*instr_position_in_region) * instr_buffer->count);

	// Now append each instruction to the corresponding region.
	// 
	// Instructions are appended in the same order they appear in the `instr_buffer`. After
	// appending the control instruction to this region, we might still encounter some instruction
	// that should also belong in this region.
	//
	// These instructions are defined later in the `instr_buffer`, and were uplifted to this region.
	// We shouldn't simply add these uplifted instruction after the control instruction. The control
	// instruction must be the last one in the region.
	//
	// To handle this correctly, during the first pass, control instructions are skipped, and later
	// during the second pass they are added to the end of each region.
	BitArray already_assigned = bit_array_alloc(temp_allocator, instr_buffer->count);
	bit_array_clear(&already_assigned);

	for (uint16_t i = 0; i < instr_buffer->count; i += 1) {
		if (states[i].decided_region_id == UINT16_MAX) {
			continue;
		}

		if (bit_array_get(&already_assigned, i)) {
			continue;
		}
		
#if 0
		if (instr_is_control(instr_buffer, (InstrIndex) { i })) {
			continue;
		}
#endif

		const Instr* instr = &instr_buffer->instr[i];

		uint16_t region_id = states[i].decided_region_id;
		InstrIndexArray* region_instr_array = &scheduled_instr_per_region[region_id];

#if 0
		if (instr->kind == INSTR_PHI) {
			InstrInputs variants = instr->phi.variants;
			for (uint16_t i = 0; i < variants.count; i += 1) {
				InstrIndex select_index = instr_buffer->inputs_buffer[variants.start + i];

				if (bit_array_get(&already_assigned, select_index.value)) {
					continue;
				}

				uint16_t region_of_select = states[select_index.value].decided_region_id;
				if (region_of_select != region_id) {
					continue;
				}

				instr_scheduled_region[select_index.value] = region_id;
				instr_position_in_region[select_index.value] = region_instr_array->count;

				region_instr_array->instr[region_instr_array->count] = select_index;
				region_instr_array->count += 1;

				bit_array_set(&already_assigned, select_index.value, true);
			}
		}
#endif

		bit_array_set(&already_assigned, i, true);

		instr_scheduled_region[i] = region_id;
#if 0
		instr_position_in_region[i] = region_instr_array->count;
#endif

		region_instr_array->instr[region_instr_array->count] = (InstrIndex) { i };
		region_instr_array->count += 1;
	}

#if 0
	// Now the second pass. Append control instructions.
	for (uint16_t i = 0; i < scheduled_regions.count; i += 1) {
		InstrIndex region_index = scheduled_regions.instr[i];
		const Instr* instr = instr_buffer_at(instr_buffer, region_index);

		InstrIndex last_instr = instr->region.last_instr;
		uint32_t region_id = instr->region.id;

		InstrIndexArray* region_instr_array = &scheduled_instr_per_region[region_id];
		assert(region_instr_array->count + 1 == instr_count_per_region[region_id]);

		instr_scheduled_region[last_instr.value] = region_id;
		instr_position_in_region[last_instr.value] = region_instr_array->count;

		region_instr_array->instr[region_instr_array->count] = last_instr;
		region_instr_array->count += 1;
	}
#endif

	for (uint16_t i = 0; i < scheduled_regions.count; i += 1) {
		uint16_t region_id = instr_region_id(instr_buffer, scheduled_regions.instr[i]);

		InstrIndexArray region_instr_array = scheduled_instr_per_region[region_id];

		printf("region %u:\n", region_id);
		for (size_t j = 0; j < region_instr_array.count; j += 1) {
			uint16_t k = region_instr_array.instr[j].value;
			Instr instr = instr_buffer->instr[k];

			printf("\t%%%u %.*s:\t\tregion=%u\torder=%u\n",
					k,
					STR_FMT(instr_name(instr.kind)),
					states[k].decided_region_id,
					states[k].order_in_region);
		}
	}

	for (uint16_t i = 0; i < scheduled_regions.count; i += 1) {
		uint32_t region_id = instr_region_id(instr_buffer, scheduled_regions.instr[i]);

		InstrIndexArray region_instr_array = scheduled_instr_per_region[region_id];
		_sort_scheduled_instr(region_instr_array, states);

		for (size_t j = 0; j < region_instr_array.count; j += 1) {
			InstrIndex instr = region_instr_array.instr[j];

			instr_position_in_region[instr.value] = (uint16_t)j;
		}
	}

	arena_end_temp(temp);

	out_result->instr_scheduled_region = instr_scheduled_region;
	out_result->scheduled_instr = scheduled_instr_per_region;
	out_result->instr_position_in_region = instr_position_in_region;

	profile_scope_end();
}

static void _schedule_regions(const InstrBuffer* instr_buffer,
		InstrIndex region_instr_index,
		Arena* allocator,
		BitArray* visited_regions,
		InstrIndexArray* out_scheduled) {
	profile_scope_start(__func__);

	const Instr* instr = instr_buffer_at(instr_buffer, region_instr_index);
	assert(instr->kind == INSTR_REGION);

	uint16_t region_id = instr->region.id;
	if (bit_array_get(visited_regions, region_id)) {
		profile_scope_end();
		return;
	}

	bit_array_set(visited_regions, region_id, true);

	const Instr* last_instr = instr_buffer_at(instr_buffer, instr->region.last_instr);
	switch (last_instr->kind) {
	case INSTR_JUMP:
		_schedule_regions(instr_buffer,
				last_instr->jump.target_region,
				allocator,
				visited_regions,
				out_scheduled);
		break;
	case INSTR_BRANCH:
		_schedule_regions(instr_buffer,
				last_instr->branch.true_region,
				allocator,
				visited_regions,
				out_scheduled);
		_schedule_regions(instr_buffer,
				last_instr->branch.false_region,
				allocator,
				visited_regions,
				out_scheduled);
		break;
	case INSTR_RET:
	case INSTR_RETURN_VALUE:
		break;
	default:
		unreachable();
	}

	*arena_alloc(allocator, InstrIndex) = region_instr_index;
	out_scheduled->count += 1;

	profile_scope_end();
}

static InstrIndexArray _gather_scheduled_regions(X64CodeGenerator* gen, InstrIndex initial_region) {
	profile_scope_start(__func__);
	
	BitArray visited_regions = bit_array_alloc(gen->temp_allocator, gen->instr_buffer.region_count);
	bit_array_clear(&visited_regions);

	InstrIndexArray regions;
	regions.instr = arena_alloc_array(gen->temp_allocator, InstrIndex, 0);
	regions.count = 0;

	_schedule_regions(&gen->instr_buffer,
			initial_region,
			gen->temp_allocator,
			&visited_regions,
			&regions);

	for (size_t i = 0; i < regions.count / 2; i += 1) {
		size_t j = regions.count - 1 - i;

		InstrIndex temp = regions.instr[i];
		regions.instr[i] = regions.instr[j];
		regions.instr[j] = temp;
	}

	profile_scope_end();
	return regions;
}

MachineCodeBuffer x64_generate_code(X64CodeGenerator* gen, InstrIndex root_region) {
	profile_scope_start(__func__);

	_init_storage_requiremenets();

	encoding_init();
	ArenaRegion temp = arena_begin_temp(gen->temp_allocator);

	// use `temp_allocator` as a persitent allocator and `allocator` is a temporary
	gen->phi_sizes = _arrange_phis_for_size_computation(&gen->instr_buffer,
			// Only used within the backend -> can use `temp_allocator` as a persistent
			gen->temp_allocator,
			gen->allocator);

	bool validation_result = _x64_validate(gen);
	assert(validation_result);

	_merge_string_consts(gen);

	CFGDominatorTree dom_tree = _build_cfg_dominator_tree(&gen->instr_buffer,
			root_region,
			gen->allocator,
			gen->temp_allocator);

	if (has_flag(gen->flags, X64_DEBUG_LOG)) {
		_print_dom_tree(&gen->instr_buffer, dom_tree);
	}

	InstrIndexArray scheduled_regions = _gather_scheduled_regions(gen, root_region);
	_collect_phis(gen, gen->temp_allocator);

	SchedulingResult scheduling_result = {};
	_schedule_instr(&gen->instr_buffer,
			scheduled_regions,
			&dom_tree,
			&scheduling_result,
			gen->temp_allocator,
			gen->allocator);

	gen->live_ranges = instr_compute_live_ranges(gen->instr_buffer,
			root_region,
			scheduling_result.scheduled_instr,
			// use `temp_allocator` as a persistent, since live ranges aren't needed outside this
			// function
			gen->temp_allocator, 
			gen->allocator);

	// Print scheduled instructions
	if (has_flag(gen->flags, X64_PRINT_SCHEDULED_IR)) {
		const InstrBuffer* instr_buffer = &gen->instr_buffer;

		for (size_t i = 0; i < scheduled_regions.count; i += 1) {
			InstrIndex region_instr = scheduled_regions.instr[i];
			const Instr* instr = instr_buffer_at(instr_buffer, region_instr);

			InstrIndexArray scheduled = scheduling_result.scheduled_instr[instr->region.id];

			printf("region %%%u id=%u: \n", (uint32_t)region_instr.value, (uint32_t)instr->region.id);
			for (size_t j = 0; j < scheduled.count; j++) {
				ArenaRegion temp = arena_begin_temp(gen->temp_allocator);
				InstrIndex instr_index = scheduled.instr[j];

				printf("%zu\t%%%u:", j, (uint32_t)instr_index.value);
				printf("\033[20G");
				instr_print(&gen->instr_buffer.instr[instr_index.value],
						gen->instr_buffer.inputs_buffer,
						gen->temp_allocator);

				arena_end_temp(temp);
			}
			printf("\n");
		}
	}

	// Now check that scheduling is valid
	bool scheduling_is_valid = true;
	for (size_t i = 0; i < scheduled_regions.count; i += 1) {
		InstrIndex region_instr = scheduled_regions.instr[i];
		const Instr* instr = &gen->instr_buffer.instr[region_instr.value];

		InstrIndexArray scheduled = scheduling_result.scheduled_instr[instr->region.id];
		scheduling_is_valid &= _validate_instr_scheduling_for_region(&gen->instr_buffer,
				scheduling_result.instr_scheduled_region,
				scheduling_result.instr_position_in_region,
				instr->region.id,
				scheduled,
				&dom_tree,
				gen->temp_allocator);
	}

	assert_msg(scheduling_is_valid, "Instruction scheduler failed to produce a valid result");

	if (!has_flag(gen->flags, X64_SKIP_REG_ALLOC)) {
		_run_reg_allocator(gen);
	}

	uint16_t region_count = gen->instr_buffer.region_count;
	gen->per_region_code_buffer = arena_alloc_array_zeroed(gen->temp_allocator,
			CodeBuffer,
			region_count);

	for (size_t i = 0; i < scheduled_regions.count; i += 1) {
		InstrIndex region_instr = scheduled_regions.instr[i];
		const Instr* instr = &gen->instr_buffer.instr[region_instr.value];

		CodeBuffer* code_buffer = &gen->per_region_code_buffer[instr->region.id];
		code_buffer_init(code_buffer, gen->temp_allocator);

		InstrIndexArray scheduled = scheduling_result.scheduled_instr[instr->region.id];
		for (size_t j = 0; j < scheduled.count; j += 1) {
			_lower_instr(gen, scheduled.instr[j], instr->region.id, code_buffer);
		}
	}

	size_t final_code_size = 0;
	size_t* code_block_offsets = arena_alloc_array(gen->temp_allocator, size_t, region_count);
	size_t* control_instr_size = arena_alloc_array(gen->temp_allocator, size_t, region_count);
	for (uint16_t i = 0; i < scheduled_regions.count; i += 1) {
		InstrBuffer* instr_buffer = &gen->instr_buffer;

		const Instr* region_instr = instr_buffer_at(instr_buffer, scheduled_regions.instr[i]);
		uint16_t region_id = instr_region_id(&gen->instr_buffer, scheduled_regions.instr[i]);

		const CodeBuffer* code_buffer = &gen->per_region_code_buffer[region_id];
		code_block_offsets[region_id] = final_code_size;
		control_instr_size[region_id] = _compute_control_instr_encoding_size(
				instr_buffer_at(instr_buffer, region_instr->region.last_instr));

		final_code_size += code_buffer->size;
		final_code_size += control_instr_size[region_id];
	}

	void* executable_memory = allocate_executable(final_code_size);
	for (uint16_t i = 0; i < scheduled_regions.count; i += 1) {
		InstrBuffer* instr_buffer = &gen->instr_buffer;

		const Instr* region_instr = instr_buffer_at(instr_buffer, scheduled_regions.instr[i]);
		uint16_t region_id = instr_region_id(&gen->instr_buffer, scheduled_regions.instr[i]);

		size_t block_size = gen->per_region_code_buffer[region_id].size;
		size_t block_offset = code_block_offsets[region_id];

		memcpy((uint8_t*)executable_memory + block_offset,
				gen->per_region_code_buffer[region_id].buffer,
				block_size);

		CodeBuffer control_instr_buffer = {};
		code_buffer_wrap(&control_instr_buffer,
				(uint8_t*)executable_memory + block_offset + block_size,
				control_instr_size[region_id]);

		_encode_control_instr(
				instr_buffer_at(instr_buffer, region_instr->region.last_instr),
				instr_buffer,
				block_offset + block_size,
				code_block_offsets,
				&control_instr_buffer);

		assert(control_instr_buffer.size == control_instr_buffer.capacity);
	}

	const InstrBuffer* instr_buffer = &gen->instr_buffer;
	size_t entry_point_offset = code_block_offsets[instr_region_id(instr_buffer, root_region)];
	assert(entry_point_offset == 0);

	MachineCodeBuffer machine_code;
	machine_code.code = executable_memory;
	machine_code.size_in_bytes = final_code_size;

	arena_end_temp(temp);
	profile_scope_end();
	return machine_code;
}
