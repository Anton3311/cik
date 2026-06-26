#include "x64.h"

#include "core/profiler.h"
#include "code_gen/backends/x64_reg_alloc.h"

inline uint8_t _bit_count_from_index(uint8_t i) {
	return (1 << i) * 8;
}

X64InstrStorageRequirement s_instr_storage_requiremenets[INSTR_COUNT] = {
	[INSTR_NO_OP]                  = (X64InstrStorageRequirement) { .allowed_registers = 0, .reg_size = 0 },

	[INSTR_CONST_8]                = (X64InstrStorageRequirement) { .allowed_registers = UINT16_MAX, .reg_size = 8 },
	[INSTR_CONST_16]               = (X64InstrStorageRequirement) { .allowed_registers = UINT16_MAX, .reg_size = 16 },
	[INSTR_CONST_32]               = (X64InstrStorageRequirement) { .allowed_registers = UINT16_MAX, .reg_size = 32 },
	[INSTR_CONST_64]               = (X64InstrStorageRequirement) { .allowed_registers = UINT16_MAX, .reg_size = 64 },

	[INSTR_CONST_STRING]           = (X64InstrStorageRequirement) { .allowed_registers = UINT16_MAX, .reg_size = 64 },

	[INSTR_BIN_OP_8]               = (X64InstrStorageRequirement) { .allowed_registers = UINT16_MAX, .reg_size = 8 },
	[INSTR_BIN_OP_16]              = (X64InstrStorageRequirement) { .allowed_registers = UINT16_MAX, .reg_size = 16 },
	[INSTR_BIN_OP_32]              = (X64InstrStorageRequirement) { .allowed_registers = UINT16_MAX, .reg_size = 32 },
	[INSTR_BIN_OP_64]              = (X64InstrStorageRequirement) { .allowed_registers = UINT16_MAX, .reg_size = 64 },

	[INSTR_LOGICAL_SHIFT_LEFT_8]   = (X64InstrStorageRequirement) { .allowed_registers = UINT16_MAX, .reg_size = 8 },
	[INSTR_LOGICAL_SHIFT_LEFT_16]  = (X64InstrStorageRequirement) { .allowed_registers = UINT16_MAX, .reg_size = 16 },
	[INSTR_LOGICAL_SHIFT_LEFT_32]  = (X64InstrStorageRequirement) { .allowed_registers = UINT16_MAX, .reg_size = 32 },
	[INSTR_LOGICAL_SHIFT_LEFT_64]  = (X64InstrStorageRequirement) { .allowed_registers = UINT16_MAX, .reg_size = 64 },

	[INSTR_LOGICAL_SHIFT_RIGHT_8]  = (X64InstrStorageRequirement) { .allowed_registers = UINT16_MAX, .reg_size = 8 },
	[INSTR_LOGICAL_SHIFT_RIGHT_16] = (X64InstrStorageRequirement) { .allowed_registers = UINT16_MAX, .reg_size = 16 },
	[INSTR_LOGICAL_SHIFT_RIGHT_32] = (X64InstrStorageRequirement) { .allowed_registers = UINT16_MAX, .reg_size = 32 },
	[INSTR_LOGICAL_SHIFT_RIGHT_64] = (X64InstrStorageRequirement) { .allowed_registers = UINT16_MAX, .reg_size = 64 },

	[INSTR_COMPARE_8]              = (X64InstrStorageRequirement) { .allowed_registers = 0, .reg_size = 0 },
	[INSTR_COMPARE_16]             = (X64InstrStorageRequirement) { .allowed_registers = 0, .reg_size = 0 },
	[INSTR_COMPARE_32]             = (X64InstrStorageRequirement) { .allowed_registers = 0, .reg_size = 0 },
	[INSTR_COMPARE_64]             = (X64InstrStorageRequirement) { .allowed_registers = 0, .reg_size = 0 },

	[INSTR_BOOL_TO_INT]            = (X64InstrStorageRequirement) { .allowed_registers = UINT16_MAX, .reg_size = 8 },

	[INSTR_NEGATE_8]               = (X64InstrStorageRequirement) { .allowed_registers = UINT16_MAX, .reg_size = 8 },
	[INSTR_NEGATE_16]              = (X64InstrStorageRequirement) { .allowed_registers = UINT16_MAX, .reg_size = 8 },
	[INSTR_NEGATE_32]              = (X64InstrStorageRequirement) { .allowed_registers = UINT16_MAX, .reg_size = 8 },
	[INSTR_NEGATE_64]              = (X64InstrStorageRequirement) { .allowed_registers = UINT16_MAX, .reg_size = 8 },

	[INSTR_CAST_TO_8]              = (X64InstrStorageRequirement) { .allowed_registers = UINT16_MAX, .reg_size = 8 },
	[INSTR_CAST_TO_16]             = (X64InstrStorageRequirement) { .allowed_registers = UINT16_MAX, .reg_size = 16 },
	[INSTR_CAST_TO_32]             = (X64InstrStorageRequirement) { .allowed_registers = UINT16_MAX, .reg_size = 32 },
	[INSTR_CAST_TO_64]             = (X64InstrStorageRequirement) { .allowed_registers = UINT16_MAX, .reg_size = 64 },

	[INSTR_PTR_LOAD_8]             = (X64InstrStorageRequirement) { .allowed_registers = UINT16_MAX, .reg_size = 8 },
	[INSTR_PTR_LOAD_16]            = (X64InstrStorageRequirement) { .allowed_registers = UINT16_MAX, .reg_size = 16 },
	[INSTR_PTR_LOAD_32]            = (X64InstrStorageRequirement) { .allowed_registers = UINT16_MAX, .reg_size = 32 },
	[INSTR_PTR_LOAD_64]            = (X64InstrStorageRequirement) { .allowed_registers = UINT16_MAX, .reg_size = 64 },

	[INSTR_LOAD_ARG]               = (X64InstrStorageRequirement) { .allowed_registers = UINT16_MAX, .reg_size = 64 },

	[INSTR_BRANCH]                 = (X64InstrStorageRequirement) { .allowed_registers = 0, .reg_size = 0 },
	[INSTR_JUMP]                   = (X64InstrStorageRequirement) { .allowed_registers = 0, .reg_size = 0 },

	[INSTR_RETURN_VALUE]           = (X64InstrStorageRequirement) { .allowed_registers = 0, .reg_size = 0 },

	[INSTR_CALL_INTERNAL]          = (X64InstrStorageRequirement) { .allowed_registers = UINT16_MAX, .reg_size = 64 },

	[INSTR_REGION]                 = (X64InstrStorageRequirement) { .allowed_registers = 0, .reg_size = 0 },

	[INSTR_PHI]                    = (X64InstrStorageRequirement) { .allowed_registers = UINT16_MAX, .reg_size = 64 },
	[INSTR_SELECT]                 = (X64InstrStorageRequirement) { .allowed_registers = 0, .reg_size = 0 },
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

static void _gather_phis(X64CodeGenerator* gen) {
	profile_scope_start(__func__);
	ArenaRegion temp = arena_begin_temp(gen->temp_allocator);

	uint16_t* phi_variant_counts_per_region = arena_alloc_array_zeroed(gen->allocator,
			uint16_t,
			gen->instr_buffer.region_count);

	for (uint16_t i = 0; i < gen->instr_buffer.count; i += 1) {
		const Instr* instr = &gen->instr_buffer.instr[i];
		if (instr->kind == INSTR_PHI) {
			InstrInputs variants = instr->phi.variants;
			for (uint16_t j = 0; j < variants.count; j += 1) {
				InstrIndex variant_index = gen->instr_buffer.inputs_buffer[variants.start + j];
				const Instr* variant = &gen->instr_buffer.instr[variant_index.value];
				assert(variant->kind == INSTR_SELECT);

				uint16_t region_id = instr_region_id(&gen->instr_buffer, variant->select.region);
				phi_variant_counts_per_region[region_id] += 1;
			}
		}
	}

	InstrIndexArray* phi_variants_per_region = arena_alloc_array_zeroed(gen->allocator,
			InstrIndexArray,
			gen->instr_buffer.region_count);
	InstrIndex** phi_node_of_variant = arena_alloc_array_zeroed(gen->allocator,
			InstrIndex*,
			gen->instr_buffer.region_count);

	for (uint16_t i = 0 ;i < gen->instr_buffer.region_count; i += 1) {
		phi_variants_per_region[i].instr = arena_alloc_array(gen->allocator,
				InstrIndex,
				phi_variant_counts_per_region[i]);

		phi_node_of_variant[i] = arena_alloc_array(gen->allocator,
				InstrIndex,
				phi_variant_counts_per_region[i]);
	}

	for (uint16_t i = 0; i < gen->instr_buffer.count; i += 1) {
		const Instr* instr = &gen->instr_buffer.instr[i];
		if (instr->kind == INSTR_PHI) {
			InstrInputs variants = instr->phi.variants;
			for (uint16_t j = 0; j < variants.count; j += 1) {
				InstrIndex variant_index = gen->instr_buffer.inputs_buffer[variants.start + j];
				const Instr* variant = &gen->instr_buffer.instr[variant_index.value];
				assert(variant->kind == INSTR_SELECT);

				uint16_t region_id = instr_region_id(&gen->instr_buffer, variant->select.region);
				uint16_t variant_count_in_region = phi_variants_per_region[region_id].count;

				assert(variant_count_in_region < phi_variant_counts_per_region[region_id]);
				phi_variants_per_region[region_id].instr[variant_count_in_region] = variant->select.value;
				phi_variants_per_region[region_id].count += 1;

				phi_node_of_variant[region_id][variant_count_in_region] = (InstrIndex) { .value = i };
			}
		}
	}

	gen->phi_variant_counts_per_region = phi_variant_counts_per_region;
	gen->phi_variants_per_region = phi_variants_per_region;
	gen->phi_node_of_variant = phi_node_of_variant;

	if (has_flag(gen->flags, X64_DEBUG_LOG)) {
		printf("phi_variants_per_region:\n");
		for (uint16_t i = 0 ;i < gen->instr_buffer.region_count; i += 1) {
			printf("%u:\n", (uint32_t)i);
			for (uint16_t j = 0; j < phi_variant_counts_per_region[i]; j += 1) {
				printf("  phi: %u variant_value: %u\n",
						(uint32_t)phi_node_of_variant[i][j].value,
						(uint32_t)phi_variants_per_region[i].instr[j].value);
			}
		}
	}

	arena_end_temp(temp);
	profile_scope_end();
}

static void _run_reg_allocator(X64CodeGenerator* gen) {
	uint16_t allowed_registers = UINT16_MAX;
	allowed_registers &= ~(1 << X64_REG_SP);
	allowed_registers &= ~(1 << X64_REG_BP);

	// HACK: Some times the register allocator might allocate the whole register
	//       to some instruction and also it's high part to the other, thus any
	//       writes by any of the two instructions will be reflected in two places.
	allowed_registers &= ~(1 << X64_REG_SI);
	allowed_registers &= ~(1 << X64_REG_DI);

	gen->instr_storage = x64_alloc_regs(&gen->instr_buffer,
			gen->usage_ranges,
			allowed_registers,
			gen->allocator,
			gen->temp_allocator);

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

//
// Code Generation
//

static void _x64_generate_code(X64CodeGenerator* gen, InstrIndex instr_index, CodeBuffer* buffer);
static void _x64_generate_phi_variants(X64CodeGenerator* gen, uint16_t region_id, CodeBuffer* code_buffer) {
	uint16_t phi_variant_count = gen->phi_variant_counts_per_region[region_id];

	const InstrIndexArray phi_variants = gen->phi_variants_per_region[region_id];
	for (uint16_t i = 0; i < phi_variant_count; i += 1) {
		InstrIndex variant_index = phi_variants.instr[i];
		_x64_generate_code(gen, variant_index, code_buffer);
	}
}

static void _x64_generate_phi_copies(X64CodeGenerator* gen, uint16_t region_id, CodeBuffer* code_buffer) {
	uint16_t phi_variant_count = gen->phi_variant_counts_per_region[region_id];

	const InstrIndexArray phi_variants = gen->phi_variants_per_region[region_id];
	const InstrIndex* phi_nodes = gen->phi_node_of_variant[region_id];

	for (uint16_t i = 0; i < phi_variant_count; i += 1) {
		InstrIndex variant_index = phi_variants.instr[i];
		InstrIndex phi_node_index = phi_nodes[i];
		const Instr* value = &gen->instr_buffer.instr[variant_index.value];

		const InstrStorageLocation value_storage = gen->instr_storage[phi_variants.instr[i].value];
		const InstrStorageLocation phi_storage = gen->instr_storage[phi_node_index.value];

		assert(value_storage.kind == INSTR_STORAGE_REG);
		assert(phi_storage.kind == INSTR_STORAGE_REG);

		uint8_t value_bit_size = s_instr_storage_requiremenets[value->kind].reg_size;
		_emit_mov_regs(code_buffer, value_storage.reg, phi_storage.reg, value_bit_size);
	}
}

void _x64_generate_code(X64CodeGenerator* gen, InstrIndex instr_index, CodeBuffer* buffer) {
	assert(instr_index.value < gen->instr_buffer.count);

	const InstrBuffer* instr_buffer = &gen->instr_buffer;
	const Instr* instr = &gen->instr_buffer.instr[instr_index.value];
	const InstrStorageLocation instr_storage = gen->instr_storage[instr_index.value];

	switch (instr->kind) {
	case INSTR_NO_OP:
		return;

	case INSTR_CONST_8:
		assert(instr_storage.kind == INSTR_STORAGE_REG);
		_emit_load_const_8(buffer, instr_storage.reg, instr->const_8.u);
		return;
	case INSTR_CONST_16:
		unreachable();
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
		assert_msg(instr->kind != INSTR_BIN_OP_16, "Not implemented");

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
			break;
		case INSTR_BIN_SUB: {
			bool should_save_right = dst_loc.reg == right_loc.reg;

			if (should_save_right) {
				// NOTE: When saving the register, push/pop the whole 64-bit register
				encode_1(buffer,
						MNEMONIC_PUSH,
						operand_reg(right_loc.reg, 64));
			}

			encode_2(buffer,
					MNEMONIC_SUB,
					operand_reg(left_reg, bit_count),
					operand_reg(right_reg, bit_count));

			if (should_save_right) {
				_emit_mov_regs(buffer, left_loc.reg, right_loc.reg, bit_count);

				encode_1(buffer,
						MNEMONIC_POP,
						operand_reg(right_loc.reg, 64));
			}
			break;
		}
		}

		return;
	}

	case INSTR_PTR_LOAD_8:
	case INSTR_PTR_LOAD_16:
	case INSTR_PTR_LOAD_32:
	case INSTR_PTR_LOAD_64: {
		const InstrStorageLocation dst_loc = gen->instr_storage[instr_index.value];
		const InstrStorageLocation ptr_loc = gen->instr_storage[instr->ptr_load.ptr.value];
		assert(dst_loc.kind == INSTR_STORAGE_REG);
		assert(ptr_loc.kind == INSTR_STORAGE_REG);

		uint8_t bit_count = _bit_count_from_index(instr->kind - INSTR_PTR_LOAD_8);
		encode_2(buffer,
				MNEMONIC_MOV,
				operand_reg(dst_loc.reg, bit_count),
				operand_mem(ptr_loc.reg, bit_count));
		return;
	}

	case INSTR_LOAD_ARG:
		// There is nothing to do. These instruction type is more of a hint
		// to where to look for the value, it doesn't get turned into any machine code.
		//
		// The register allocator allocates registers corresponding to the function
		// argument, and during the compilation of other instructions, the allocated
		// register is used as an input.
		return;

	case INSTR_LOGICAL_SHIFT_LEFT_8:
	case INSTR_LOGICAL_SHIFT_LEFT_16:
	case INSTR_LOGICAL_SHIFT_LEFT_32:
		unreachable();
	case INSTR_LOGICAL_SHIFT_LEFT_64: {
		const InstrStorageLocation dst_loc = gen->instr_storage[instr_index.value];
		const InstrStorageLocation operand_loc = gen->instr_storage[instr->logical_shift.operand.value];

		assert(dst_loc.kind == INSTR_STORAGE_REG);
		assert(operand_loc.kind == INSTR_STORAGE_REG);

		_emit_mov_regs(buffer, operand_loc.reg, dst_loc.reg, 64);

		encode_2(buffer,
				MNEMONIC_SHL,
				operand_reg(dst_loc.reg, 64),
				operand_imm(instr->logical_shift.shift_count, 8));
		return;
	}

	case INSTR_COMPARE_8:
	case INSTR_COMPARE_16:
	case INSTR_COMPARE_32:
	case INSTR_COMPARE_64: {
		if (instr->kind == INSTR_COMPARE_16) {
			unreachable();
		}

		const InstrStorageLocation left_loc = gen->instr_storage[instr->bin_op.left.value];
		const InstrStorageLocation right_loc = gen->instr_storage[instr->bin_op.right.value];
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
		assert(operand_instr->kind >= INSTR_COMPARE_8);
		assert(operand_instr->kind <= INSTR_COMPARE_64);

		InstrCompareKind compare_kind = operand_instr->compare.kind;
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
		assert(instr->kind != INSTR_NEGATE_16);

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

		uint8_t operand_size = s_instr_storage_requiremenets[value->kind].reg_size;
		uint8_t output_size = 8 << (instr->kind - INSTR_CAST_TO_8);

		assert_msg(operand_size != 16, "16-bit not yet implemented");
		assert_msg(output_size != 16, "16-bit not yet implemented");

		assert(dst_loc.kind == INSTR_STORAGE_REG);
		assert(src_loc.kind == INSTR_STORAGE_REG);

		if (operand_size == output_size) {
			// The cast is redundant
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

		if (operand_size == 8) {
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

		// HACK: Need to store somewhere the currently processed region
		uint16_t current_region_id = (uint16_t)(buffer - gen->per_region_code_buffer);

		switch (condition_instr->kind) {
		case INSTR_COMPARE_8:
		case INSTR_COMPARE_16:
		case INSTR_COMPARE_32:
		case INSTR_COMPARE_64:
			break;
		default:
			const InstrStorageLocation cond_loc = gen->instr_storage[instr->branch.condition.value];
			assert(cond_loc.kind == INSTR_STORAGE_REG);

			assert(has_flag(INSTR_FEATURES[condition_instr->kind], INSTR_FEATURE_REG_STORAGE));
			uint8_t bit_count = s_instr_storage_requiremenets[condition_instr->kind].reg_size;
			encode_2(buffer,
					MNEMONIC_TEST,
					operand_reg(cond_loc.reg, bit_count),
					operand_reg(cond_loc.reg, bit_count));
			break;
		}

		_x64_generate_phi_copies(gen, current_region_id, buffer);
		return;
	}
	case INSTR_JUMP: {
		// HACK: Need to store somewhere the currently processed region
		uint16_t current_region_id = (uint16_t)(buffer - gen->per_region_code_buffer);

		_x64_generate_phi_copies(gen, current_region_id, buffer);
		return;
	}

	case INSTR_RETURN_VALUE:
		const InstrStorageLocation return_value_loc = gen->instr_storage[instr->return_value.value.value];
		assert(return_value_loc.kind == INSTR_STORAGE_REG);

		_emit_mov_regs(buffer, return_value_loc.reg, X64_REG_A, 64);

		// NOTE: Don't need to generate a `ret` instruction, since it is done later when the
		//       control instructions at the end of each code blocks are generated
		return;
	case INSTR_RET:
		// NOTE: Don't need to generate a `ret` instruction, since it is done later when the
		//       control instructions at the end of each code blocks are generated
		return;
	
	case INSTR_IO_STATE:
		return;
	
	case INSTR_CALL_INTERNAL: {
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

		assert(instr->call_internal.args.count <= 1);

		InstrInputs args = instr->call_internal.args;

		// Push saved registers
		for (size_t i = 0; i < array_size(saved_registers); i += 1) {
			encode_1(buffer,
					MNEMONIC_PUSH,
					operand_reg(saved_registers[i], 64));
		}

		if (args.count == 1) {
			InstrIndex arg_instr = gen->instr_buffer.inputs_buffer[args.start + 0];
			const InstrStorageLocation arg_storage_loc = gen->instr_storage[arg_instr.value];
			_emit_mov_regs(buffer, arg_storage_loc.reg, X64_REG_C, 64);
		}

		uint16_t function_id = instr->call_internal.function_index;
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
	case INSTR_SELECT: {
		return;
	}
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
		switch (condition_instr->kind) {
		case INSTR_COMPARE_8:
		case INSTR_COMPARE_16:
		case INSTR_COMPARE_32:
		case INSTR_COMPARE_64:
			jump_to_true_mnemonic_kind = _select_jmp_mnemonic(condition_instr->compare.kind);
			break;
		default:
			jump_to_true_mnemonic_kind = MNEMONIC_JNZ;
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

static void _validate_linearization(const InstrBuffer* instr_buffer,
		BitArray* visited_instr,
		InstrIndexArray linearized,
		Arena* temp_allocator) {

	ArenaRegion temp = arena_begin_temp(temp_allocator);
	for (size_t i = 0; i < linearized.count; i += 1) {
		ArenaRegion inner_temp = arena_begin_temp(temp_allocator);

		InstrQueue queue;
		instr_queue_alloc(&queue, temp_allocator, instr_buffer->count);

		instr_enumerate_uses(instr_buffer, linearized.instr[i], &queue);

		for (size_t j = 0; j < queue.count; j += 1) {
			InstrIndex input = queue.buffer[j];
			if (input.value == INVALID_INSTR_INDEX.value) {
				continue;
			}

			const Instr* input_instr = instr_buffer_at(instr_buffer, input);
			bool skip = false;
			switch (input_instr->kind) {
			case INSTR_REGION:
				skip = true;
				break;
			}

			if (skip) {
				continue;
			}

			assert_msg(bit_array_get(visited_instr, input.value),
					"Value definition '%u' appears before its input '%u'",
					(uint32_t)linearized.instr[i].value,
					(uint32_t)input.value);
		}

		bit_array_set(visited_instr, linearized.instr[i].value, true);

		arena_end_temp(inner_temp);
	}
	
	arena_end_temp(temp);
}

static void _linearize_instr(X64CodeGenerator* gen,
		InstrIndex instr_index,
		BitArray* visited_instr,
		InstrIndexArray* out_linearized,
		Arena* allocator);

static void _linearize_phis(X64CodeGenerator* gen,
		BitArray* visited_instr,
		InstrIndexArray* out_linearized,
		Arena* temp_allocator) {

	const InstrBuffer* instr_buffer = &gen->instr_buffer;
	uint16_t region_id = gen->current_linearized_region_id; 

	uint16_t phi_variant_count = gen->phi_variant_counts_per_region[region_id];
	const InstrIndexArray phi_variants = gen->phi_variants_per_region[region_id];

	for (uint16_t i = 0; i < phi_variant_count; i += 1) {
		InstrIndex variant_index = phi_variants.instr[i];

		if (bit_array_get(visited_instr, variant_index.value)) {
			continue;
		}

		_linearize_instr(gen,
				variant_index,
				visited_instr,
				out_linearized,
				temp_allocator);
	}
}

static void _linearize_instr(X64CodeGenerator* gen,
		InstrIndex instr_index,
		BitArray* visited_instr,
		InstrIndexArray* out_linearized,
		Arena* allocator) {

	const InstrBuffer* instr_buffer = &gen->instr_buffer;
	const Instr* instr = instr_buffer_at(instr_buffer, instr_index);
	if (instr->kind == INSTR_REGION) {
		return;
	}

	if (bit_array_get(visited_instr, instr_index.value)) {
		return;
	}

	bit_array_set(visited_instr, instr_index.value, true);

	switch (instr->kind) {
	case INSTR_NO_OP:
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
		_linearize_instr(gen,
				instr->bin_op.left,
				visited_instr,
				out_linearized,
				allocator);
		_linearize_instr(gen,
				instr->bin_op.right,
				visited_instr,
				out_linearized,
				allocator);
		break;
	case INSTR_PTR_LOAD_8:
	case INSTR_PTR_LOAD_16:
	case INSTR_PTR_LOAD_32:
	case INSTR_PTR_LOAD_64:
		_linearize_instr(gen,
				instr->ptr_load.ptr,
				visited_instr,
				out_linearized,
				allocator);
		break;
	case INSTR_LOAD_ARG:
		break;
	case INSTR_LOGICAL_SHIFT_LEFT_8:
	case INSTR_LOGICAL_SHIFT_LEFT_16:
	case INSTR_LOGICAL_SHIFT_LEFT_32:
	case INSTR_LOGICAL_SHIFT_LEFT_64:
		_linearize_instr(gen,
				instr->logical_shift.operand,
				visited_instr,
				out_linearized,
				allocator);
		break;
	case INSTR_COMPARE_8:
	case INSTR_COMPARE_16:
	case INSTR_COMPARE_32:
	case INSTR_COMPARE_64:
		_linearize_instr(gen,
				instr->compare.left,
				visited_instr,
				out_linearized,
				allocator);
		_linearize_instr(gen,
				instr->compare.right,
				visited_instr,
				out_linearized,
				allocator);
		break;
	case INSTR_BOOL_TO_INT:
		_linearize_instr(gen,
				instr->bool_to_int.operand,
				visited_instr,
				out_linearized,
				allocator);
		break;
	case INSTR_NEGATE_8:
	case INSTR_NEGATE_16:
	case INSTR_NEGATE_32:
	case INSTR_NEGATE_64:
		_linearize_instr(gen,
				instr->negate.operand,
				visited_instr,
				out_linearized,
				allocator);
		break;
	case INSTR_CAST_TO_8:
	case INSTR_CAST_TO_16:
	case INSTR_CAST_TO_32:
	case INSTR_CAST_TO_64:
		_linearize_instr(gen,
				instr->cast.value,
				visited_instr,
				out_linearized,
				allocator);
		break;
	case INSTR_BRANCH:
		_linearize_instr(gen,
				instr->branch.io_state,
				visited_instr,
				out_linearized,
				allocator);
		_linearize_instr(gen,
				instr->branch.condition,
				visited_instr,
				out_linearized,
				allocator);

		_linearize_phis(gen, visited_instr, out_linearized, allocator);
		break;
	case INSTR_JUMP:
		_linearize_instr(gen,
				instr->jump.io_state,
				visited_instr,
				out_linearized,
				allocator);
		_linearize_phis(gen, visited_instr, out_linearized, allocator);
		break;
	case INSTR_RETURN_VALUE:
		_linearize_instr(gen,
				instr->return_value.io_state,
				visited_instr,
				out_linearized,
				allocator);
		_linearize_instr(gen,
				instr->return_value.value,
				visited_instr,
				out_linearized,
				allocator);
		break;
	case INSTR_RET:
		_linearize_instr(gen,
				instr->ret.io_state,
				visited_instr,
				out_linearized,
				allocator);
		break;
	case INSTR_IO_STATE:
		if (instr->io_state.producer.value != UINT16_MAX) {
			_linearize_instr(gen,
					instr->io_state.producer,
					visited_instr,
					out_linearized,
					allocator);
		}

		break;
	case INSTR_CALL_INTERNAL: {
		_linearize_instr(gen,
				instr->call_internal.io_state,
				visited_instr,
				out_linearized,
				allocator);

		InstrInputs args = instr->call_internal.args;
		if (args.count == 1) {
			InstrIndex arg_instr = instr_buffer->inputs_buffer[args.start + 0];
			_linearize_instr(gen,
					arg_instr,
					visited_instr,
					out_linearized,
					allocator);
		}

		break;
	}
	case INSTR_REGION:
		unreachable();
	case INSTR_PHI: {
		InstrInputs variants = instr->phi.variants;
		for (uint16_t i = 0; i < variants.count; i += 1) {
			InstrIndex variant = instr_buffer->inputs_buffer[variants.start + i];
			_linearize_instr(gen,
					variant,
					visited_instr,
					out_linearized,
					allocator);
		}
		break;
	}
	case INSTR_SELECT:
		_linearize_instr(gen,
				instr->select.value,
				visited_instr,
				out_linearized,
				allocator);
		break;
	}

	arena_alloc(allocator, InstrIndex);
	out_linearized->instr[out_linearized->count] = instr_index;
	out_linearized->count += 1;
}

static InstrIndexArray _linearize_instr_for_region(X64CodeGenerator* gen,
		const InstrBuffer* instr_buffer,
		uint16_t region_id,
		InstrIndex initial_instr,
		BitArray* visited_instr,
		Arena* temp_allocator) {

	InstrIndexArray linearized;
	linearized.instr = arena_alloc_array(temp_allocator, InstrIndex, 0);
	linearized.count = 0;

	_linearize_instr(gen, initial_instr, visited_instr, &linearized, temp_allocator);
	return linearized;
}

static void _schedule_regions(X64CodeGenerator* gen,
		InstrIndex region_instr_index,
		Arena* allocator,
		BitArray* visited_regions,
		InstrIndexArray* out_scheduled) {
	const InstrBuffer* instr_buffer = &gen->instr_buffer;
	const Instr* instr = instr_buffer_at(instr_buffer, region_instr_index);

	assert(instr->kind == INSTR_REGION);

	uint16_t region_id = instr->region.id;
	if (bit_array_get(visited_regions, region_id)) {
		return;
	}

	bit_array_set(visited_regions, region_id, true);

	const Instr* last_instr = instr_buffer_at(instr_buffer, instr->region.last_instr);
	switch (last_instr->kind) {
	case INSTR_JUMP:
		_schedule_regions(gen, last_instr->jump.target_region, allocator, visited_regions, out_scheduled);
		break;
	case INSTR_BRANCH:
		_schedule_regions(gen, last_instr->branch.true_region, allocator, visited_regions, out_scheduled);
		_schedule_regions(gen, last_instr->branch.false_region, allocator, visited_regions, out_scheduled);
		break;
	case INSTR_RET:
	case INSTR_RETURN_VALUE:
		break;
	}

	*arena_alloc(allocator, InstrIndex) = region_instr_index;
	out_scheduled->count += 1;
}

static InstrIndexArray _gather_scheduled_regions(X64CodeGenerator* gen, InstrIndex initial_region) {
	BitArray visited_regions = bit_array_alloc(gen->temp_allocator, gen->instr_buffer.region_count);
	bit_array_clear(&visited_regions);

	InstrIndexArray regions;
	regions.instr = arena_alloc_array(gen->temp_allocator, InstrIndex, 0);
	regions.count = 0;

	_schedule_regions(gen, initial_region, gen->temp_allocator, &visited_regions, &regions);

	for (size_t i = 0; i < regions.count / 2; i += 1) {
		size_t j = regions.count - 1 - i;

		InstrIndex temp = regions.instr[i];
		regions.instr[i] = regions.instr[j];
		regions.instr[j] = temp;
	}

	return regions;
}

typedef struct {
	// Per region `BitArray` of regions that it dominates
	// The size of this array is equal to the total number of regions
	//
	// Size of each `BitArray` is also equal to the total number of regions
	BitArray* dominates;

	// An array of size eqaul to the total number of regions.
	// Maps region id to the immediate dominator of that region.
	//
	// `UINT16_MAX` means the regions doesn't have an immediate dominator.
	// Which can only be true for the root region.
	uint16_t* immediate_dominators;
} CFGDominanceTree;

static CFGDominanceTree _build_cfg_dominator_tree(const InstrBuffer* instr_buffer,
		InstrIndex initial_region,
		Arena* allocator,
		Arena* temp_allocator) {
	ArenaRegion temp = arena_begin_temp(temp_allocator);

	BitArray visited_regions = bit_array_alloc(temp_allocator, instr_buffer->region_count);
	bit_array_clear(&visited_regions);

	InstrQueue stack;
	instr_queue_alloc(&stack, temp_allocator, instr_buffer->region_count);

	// Allocate the tree
	CFGDominanceTree tree;
	tree.dominates = arena_alloc_array(allocator, BitArray, instr_buffer->region_count);
	tree.immediate_dominators = arena_alloc_array(allocator, uint16_t, instr_buffer->region_count);

	for (uint16_t i = 0; i < instr_buffer->region_count; i += 1) {
		tree.dominates[i] = bit_array_alloc(allocator, instr_buffer->region_count);
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

			printf("dom tree visit: %u\n", (uint32_t)successor->region.id);

			if (bit_array_get(&visited_regions, successor->region.id)) {
				bit_array_and(&tree.dominates[instr->region.id],
						&tree.dominates[successor->region.id],
						&tree.dominates[successor->region.id]);
				 bit_array_set(&tree.dominates[successor->region.id], successor->region.id, true);
			} else {
				bit_array_or(&tree.dominates[instr->region.id],
						&tree.dominates[successor->region.id],
						&tree.dominates[successor->region.id]);

				bit_array_set(&visited_regions, successor->region.id, true);
			}

			instr_queue_push_back(&stack, successor_index);

			printf("update dom for id=%u: ", (uint32_t)successor->region.id);
			for (uint16_t j = 0; j < instr_buffer->region_count; j += 1) {
				if (bit_array_get(&tree.dominates[successor->region.id], j)) {
					printf("%u ", (uint32_t)j);
				}
			}
			printf("\n");
		}
	}

	for (uint16_t i = 0; i < instr_buffer->region_count; i += 1) {
		BitArray* dominance = &tree.dominates[i];
		assert(bit_array_get(dominance, i));
		bit_array_set(dominance, i, false);

		bool found = false;
		for (uint16_t j = 0; j < instr_buffer->region_count; j += 1) {
			if (bit_array_equal(dominance, &tree.dominates[j])) {
				tree.immediate_dominators[i] = j;
				found = true;
				bit_array_set(dominance, i, true);
				break;
			}
		}

		assert(found);
	}

	// The initial region dones't have an immediate dominator
	tree.immediate_dominators[instr_region_id(instr_buffer, initial_region)] = UINT16_MAX;

	arena_end_temp(temp);

	return tree;
}

static void _print_dom_tree(const InstrBuffer* instr_buffer, CFGDominanceTree tree) {
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

// Find a region where the control flow splits and later reaches both provided regions.
static uint16_t _find_control_flow_split(const CFGDominanceTree* tree,
		uint16_t region_count,
		uint16_t region_a_id,
		uint16_t region_b_id,
		Arena* temp_allocator) {
	ArenaRegion temp = arena_begin_temp(temp_allocator);
	BitArray visited_regions = bit_array_alloc(temp_allocator, region_count);
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
			return region_id.value;
		}

		bit_array_set(&visited_regions, region_id.value, true);
		instr_queue_push_back(&queue, (InstrIndex) { tree->immediate_dominators[region_id.value] });
	}

	unreachable();
	return UINT16_MAX;
}

static void _uplift_phi_nodes(X64CodeGenerator* gen,
		const CFGDominanceTree* dom_tree,
		Arena* temp_allocator) {

	ArenaRegion temp = arena_begin_temp(temp_allocator);

	uint16_t region_count = gen->instr_buffer.region_count;

	typedef struct VariantUse VariantUse;
	struct VariantUse {
		InstrIndex used_by_phi;
		uint16_t region_id;
		VariantUse* next;
	};

	VariantUse** variant_usages = arena_alloc_array_zeroed(temp_allocator,
			VariantUse*,
			gen->instr_buffer.count);

	size_t total_phi_count = 0;
	for (uint16_t i = 0; i < region_count; i += 1) {
		total_phi_count += gen->phi_variant_counts_per_region[i];

		for (uint16_t j = 0; j < gen->phi_variant_counts_per_region[i]; j += 1) {
			InstrIndex variant = gen->phi_variants_per_region[i].instr[j];
			InstrIndex phi_node = gen->phi_node_of_variant[i][j];

			VariantUse* use = arena_alloc(temp_allocator, VariantUse);
			use->used_by_phi = phi_node;
			use->region_id = i;
			use->next = variant_usages[variant.value];

			variant_usages[variant.value] = use;
		}
	}

	for (uint16_t i = 0; i < gen->instr_buffer.count; i += 1) {
		if (variant_usages[i] == NULL) {
			continue;
		}

		VariantUse* use_0 = variant_usages[i];
		if (use_0->next == NULL) {
			continue;
		}

		VariantUse* use_1 = use_0->next;
		assert(use_1->next == NULL);

		uint16_t control_flow_split = _find_control_flow_split(dom_tree,
				gen->instr_buffer.region_count,
				use_0->region_id,
				use_1->region_id,
				temp_allocator);

		printf("found control flow split at %u for phi [%u] and [%u]\n",
				(uint32_t)control_flow_split,
				(uint32_t)use_0->used_by_phi.value,
				(uint32_t)use_1->used_by_phi.value);
	}

	arena_end_temp(temp);
}

MachineCodeBuffer x64_generate_code(X64CodeGenerator* gen, InstrIndex root_region) {
	profile_scope_start(__func__);

	encoding_init();
	ArenaRegion temp = arena_begin_temp(gen->temp_allocator);

	bool validation_result = _x64_validate(gen);
	assert(validation_result);

	_merge_string_consts(gen);
	_gather_phis(gen);
	_run_reg_allocator(gen);
	
	InstrIndexArray scheduled_regions = _gather_scheduled_regions(gen, root_region);

	uint16_t region_count = gen->instr_buffer.region_count;
	gen->per_region_code_buffer = arena_alloc_array_zeroed(gen->temp_allocator,
			CodeBuffer,
			region_count);

	InstrIndexArray* linearized_instr_per_region = arena_alloc_array(gen->temp_allocator,
			InstrIndexArray,
			scheduled_regions.count);

	CFGDominanceTree dom_tree = _build_cfg_dominator_tree(&gen->instr_buffer,
			root_region,
			gen->allocator,
			gen->temp_allocator);

	if (has_flag(gen->flags, X64_DEBUG_LOG)) {
		_print_dom_tree(&gen->instr_buffer, dom_tree);
	}

	_uplift_phi_nodes(gen, &dom_tree, gen->temp_allocator);

	BitArray visited_instr = bit_array_alloc(gen->temp_allocator, gen->instr_buffer.count);
	bit_array_clear(&visited_instr);

	for (size_t i = 0; i < scheduled_regions.count; i += 1) {
		InstrIndex region_instr = scheduled_regions.instr[i];
		const Instr* instr = &gen->instr_buffer.instr[region_instr.value];
	 
		gen->current_linearized_region_id = instr->region.id;
		InstrIndexArray linearized = _linearize_instr_for_region(gen,
				&gen->instr_buffer,
				instr->region.id,
				instr->region.last_instr,
				&visited_instr,
				gen->temp_allocator);

		linearized_instr_per_region[instr->region.id] = linearized;
	}

	if (has_flag(gen->flags, X64_PRINT_SCHEDULED_IR)) {
		// Print linearized instructions
		for (size_t i = 0; i < scheduled_regions.count; i += 1) {
			InstrIndex region_instr = scheduled_regions.instr[i];
			const Instr* instr = &gen->instr_buffer.instr[region_instr.value];

			InstrIndexArray linearized = linearized_instr_per_region[instr->region.id];

			printf("region %%%u id=%u: \n", (uint32_t)region_instr.value, (uint32_t)instr->region.id);
			for (size_t j = 0; j < linearized.count; j++) {
				ArenaRegion temp = arena_begin_temp(gen->temp_allocator);
				InstrIndex instr_index = linearized.instr[j];

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

	bit_array_clear(&visited_instr);
	for (size_t i = 0; i < scheduled_regions.count; i += 1) {
		InstrIndex region_instr = scheduled_regions.instr[i];
		const Instr* instr = &gen->instr_buffer.instr[region_instr.value];

		InstrIndexArray linearized = linearized_instr_per_region[instr->region.id];
		_validate_linearization(&gen->instr_buffer, &visited_instr, linearized, gen->temp_allocator);
	}

	for (size_t i = 0; i < scheduled_regions.count; i += 1) {
		InstrIndex region_instr = scheduled_regions.instr[i];
		const Instr* instr = &gen->instr_buffer.instr[region_instr.value];

		CodeBuffer* code_buffer = &gen->per_region_code_buffer[instr->region.id];
		code_buffer_init(code_buffer, gen->temp_allocator);

		InstrIndexArray linearized = linearized_instr_per_region[instr->region.id];
		for (size_t j = 0; j < linearized.count; j += 1) {
			_x64_generate_code(gen, linearized.instr[j], code_buffer);
		}
	}

	uint16_t* blocks_in_dfs_order = arena_alloc_array(gen->allocator, uint16_t, scheduled_regions.count);
	for (size_t i = 0; i < scheduled_regions.count; i += 1) {
		InstrBuffer* instr_buffer = &gen->instr_buffer;
		const Instr* instr = instr_buffer_at(instr_buffer, scheduled_regions.instr[i]);
		blocks_in_dfs_order[i] = instr->region.id;
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

		memcpy(executable_memory + block_offset,
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
