#include "x64.h"

#include "core/profiler.h"

inline uint8_t _bit_count_from_index(uint8_t i) {
	return (1 << i) * 8;
}

static X64InstrStorageRequirement s_instr_storage_requiremenets[INSTR_COUNT] = {
	[INSTR_NO_OP]                  = (X64InstrStorageRequirement) { .allowed_registers = 0, .reg_size = 0 },

	[INSTR_CONST_8]                = (X64InstrStorageRequirement) { .allowed_registers = UINT16_MAX, .reg_size = 8 },
	[INSTR_CONST_16]               = (X64InstrStorageRequirement) { .allowed_registers = UINT16_MAX, .reg_size = 16 },
	[INSTR_CONST_32]               = (X64InstrStorageRequirement) { .allowed_registers = UINT16_MAX, .reg_size = 32 },
	[INSTR_CONST_64]               = (X64InstrStorageRequirement) { .allowed_registers = UINT16_MAX, .reg_size = 64 },

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

	[INSTR_COMPARE_8]              = (X64InstrStorageRequirement) { .allowed_registers = UINT16_MAX, .reg_size = 8 },
	[INSTR_COMPARE_16]             = (X64InstrStorageRequirement) { .allowed_registers = UINT16_MAX, .reg_size = 8 },
	[INSTR_COMPARE_32]             = (X64InstrStorageRequirement) { .allowed_registers = UINT16_MAX, .reg_size = 8 },
	[INSTR_COMPARE_64]             = (X64InstrStorageRequirement) { .allowed_registers = UINT16_MAX, .reg_size = 8 },

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

static InstrIndexArray _x64_gather_instr_with_storage_requirement(const InstrBuffer instr_buffer,
		const InstrUsageRange* usage_ranges,
		Arena* allocator) {

	InstrIndexArray result;
	result.count = 0;
	result.instr = arena_alloc_array(allocator, InstrIndex, 0);

	for (size_t i = 0; i < instr_buffer.count; i += 1) {
		const InstrKind kind = instr_buffer.instr[i].kind;
		if (!has_flag(INSTR_FEATURES[kind], INSTR_FEATURE_REG_STORAGE)) {
			continue;
		}

		if (usage_ranges[i].value == UINT32_MAX) {
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
static UInt16Array* _x64_build_interference_graph(const InstrBuffer instr_buffer,
		const InstrIndexArray instr_with_storage_requirement,
		const InstrUsageRange* usage_ranges,
		Arena* allocator) {

	// Each array stores indices into `instr_with_storage_requirement`
	UInt16Array* graph_edges = arena_alloc_array_zeroed(allocator,
			UInt16Array,
			instr_with_storage_requirement.count);

	for (size_t i = 0; i < instr_with_storage_requirement.count; i += 1) {
		UInt16Array* edges = &graph_edges[i];
		edges->values = arena_alloc_array(allocator, uint16_t, 0);
	
		InstrUsageRange usage_range_a = usage_ranges[instr_with_storage_requirement.instr[i].value];
		for (size_t j = 0; j < instr_with_storage_requirement.count; j += 1) {
			if (i == j) {
				continue;
			}

			InstrUsageRange usage_range_b = usage_ranges[instr_with_storage_requirement.instr[j].value];

			uint16_t max_start = max(usage_range_a.first_usage.value, usage_range_b.first_usage.value);
			uint16_t min_end = min(usage_range_a.last_usage.value, usage_range_b.last_usage.value);

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
static void _x64_run_graph_coloring(InstrBuffer instr_buffer,
		InstrIndexArray instr_with_storage_requirement,
		UInt16Array* interference_graph,
		InstrStorageLocation* instr_storage,
		uint16_t allowed_registers,
		Arena* temp_allocator) {
	profile_scope_start(__func__);
	ArenaRegion temp = arena_begin_temp(temp_allocator);

	uint16_t* potential_instr_registers = arena_alloc_array(temp_allocator,
			uint16_t,
			instr_buffer.count);

	for (size_t i = 0; i < instr_buffer.count; i += 1) {
		InstrKind kind = instr_buffer.instr[i].kind;

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
		InstrKind kind = instr_buffer.instr[instr_index.value].kind;

		if (kind != INSTR_LOAD_ARG) {
			continue;
		}

		const Instr* instr = &instr_buffer.instr[instr_index.value];

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

void x64_alloc_registers(X64CodeGenerator* gen, uint16_t allowed_registers) {
	profile_scope_start(__func__);
	ArenaRegion temp = arena_begin_temp(gen->temp_allocator);

	// What cranelift's register allocator does:
	// 1. For each region compute live input and output instructions.
	//     a. Phi nodes variants must appear as outputs of the corresponding regions
	// 2. Once we have the in/out bitset for each region, compute more precise live ranges
	//
	// What this allocator can do for now (regarding the phi nodes):
	// 1. Extend phi nodes live ranges to include the end of the live range of it's variants
	// 2. Allocate registers for phi nodes
	// 3. At the end of variant's live range place a move,
	//    which copies the value from the variant's location into phi's location.

	{
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

	InstrIndexArray instr_with_storage_requirement = _x64_gather_instr_with_storage_requirement(
			gen->instr_buffer,
			gen->usage_ranges,
			gen->temp_allocator);

	UInt16Array* interference_graph = _x64_build_interference_graph(gen->instr_buffer, 
			instr_with_storage_requirement,
			gen->usage_ranges,
			gen->temp_allocator);

	gen->instr_storage = arena_alloc_array_zeroed(gen->allocator,
			InstrStorageLocation,
			gen->instr_buffer.count);

	_x64_run_graph_coloring(gen->instr_buffer,
			instr_with_storage_requirement,
			interference_graph,
			gen->instr_storage,
			allowed_registers,
			gen->temp_allocator);

	printf("Interference Graph Edges for each Instr:\n");
	for (size_t i = 0; i < instr_with_storage_requirement.count; i += 1) {
		UInt16Array overlap = interference_graph[i];
		printf("%u: ", (uint32_t)instr_with_storage_requirement.instr[i].value);

		for (size_t j = 0; j < overlap.count; j += 1) {
			InstrIndex overlapping_instr = instr_with_storage_requirement.instr[overlap.values[j]];
			printf("%u ", (uint32_t)overlapping_instr.value);
		}

		printf("\n");
	}

	printf("Assigned storage locations:\n");
	for (size_t i = 0; i < gen->instr_buffer.count; i += 1) {
		ArenaRegion temp = arena_begin_temp(gen->temp_allocator);

		String storage_string = STR_LIT("none");

		if (gen->instr_storage[i].kind == INSTR_STORAGE_REG) {
			StringBuilder builder = { .arena = gen->temp_allocator };

			const InstrKind instr_kind = gen->instr_buffer.instr[i].kind;
			const X64InstrStorageRequirement storage_requirement = s_instr_storage_requiremenets[instr_kind];

			_format_reg_name(&builder, gen->instr_storage[i].reg, storage_requirement.reg_size);
			storage_string = builder.string;
		}

		printf("%zu: %.*s\n", i, STR_FMT(storage_string));

		arena_end_temp(temp);
	}

	arena_end_temp(temp);
	profile_scope_end();
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

inline uint8_t _rex_prefix(uint8_t w, uint8_t r, uint8_t x, uint8_t b) {
	assert(w <= 1);
	assert(r <= 1);
	assert(x <= 1);
	assert(b <= 1);
	return 0b01000000 | (w << 3) | (r << 2) | (x << 1) | (b << 0);
}

inline uint8_t _rex_prefix_src_dst(uint8_t is_64_bit_reg, uint8_t src_reg, uint8_t dst_reg) {
	return _rex_prefix(is_64_bit_reg, src_reg >> 3, 0, dst_reg >> 3);
}

typedef enum {
	MOD_RM_ADDRESS_RM         = 0b00000000,
	MOD_RM_ADDRESS_RM_DISP_8  = 0b00000000,
	MOD_RM_ADDRESS_RM_DISP_32 = 0b00000000,
	MOD_RM_RM                 = 0b11000000,
} ModRMMod;

inline uint8_t _mod_rm(ModRMMod mod, X64Register reg, uint8_t rm) {
	assert(reg < 8);
	assert(rm < 8);
	assert((mod & 0b00111111) == 0);
	return ((uint8_t)mod) | (reg << 3) | (rm);
}

inline uint8_t _mod_rm_with_ext(uint8_t extension, uint8_t reg) {
	assert(extension < 8);
	assert(reg < 8);
	return 0b11000000 | (extension << 3) | (reg);
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

inline void _emit_return(CodeBuffer* buffer) {
	*code_buffer_append(buffer, 1) = 0xc3;
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

	case INSTR_BIN_OP_8:
	case INSTR_BIN_OP_16:
	case INSTR_BIN_OP_32:
	case INSTR_BIN_OP_64: {
		assert_msg(instr->kind != INSTR_BIN_OP_16, "Not implemented");

		uint8_t bit_count = _bit_count_from_index(instr->kind - INSTR_BIN_OP_8);

		_x64_generate_code(gen, instr->bin_op.left, buffer);
		_x64_generate_code(gen, instr->bin_op.right, buffer);

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
		_x64_generate_code(gen, instr->ptr_load.ptr, buffer);

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
		_x64_generate_code(gen, instr->logical_shift.operand, buffer);

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

		_x64_generate_code(gen, instr->compare.left, buffer);
		_x64_generate_code(gen, instr->compare.right, buffer);

		const InstrStorageLocation dst_loc = gen->instr_storage[instr_index.value];
		const InstrStorageLocation left_loc = gen->instr_storage[instr->bin_op.left.value];
		const InstrStorageLocation right_loc = gen->instr_storage[instr->bin_op.right.value];
		assert(dst_loc.kind == INSTR_STORAGE_REG);
		assert(left_loc.kind == INSTR_STORAGE_REG);
		assert(right_loc.kind == INSTR_STORAGE_REG);

		uint8_t bit_count = _bit_count_from_index(instr->kind - INSTR_COMPARE_8);

		encode_2(buffer,
				MNEMONIC_CMP,
				operand_reg(left_loc.reg, bit_count),
				operand_reg(right_loc.reg, bit_count));

		MnemonicKind mnemonic = 0;
		switch (instr->compare.kind) {
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
	
	case INSTR_CAST_TO_8:
	case INSTR_CAST_TO_16:
	case INSTR_CAST_TO_32:
	case INSTR_CAST_TO_64: {
		_x64_generate_code(gen, instr->cast.value, buffer);

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
			// NOTE: Moving writing to a 32-bit register zeros out the upper half of the corresponding 64-bit regiters.
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
		_x64_generate_code(gen, instr->branch.io_state, buffer);

		// HACK: Need to store somewhere the currently processed region
		uint16_t current_region_id = (uint16_t)(buffer - gen->per_region_code_buffer);

		_x64_generate_phi_variants(gen, current_region_id, buffer);

		_x64_generate_code(gen, instr->branch.condition, buffer);

		_x64_generate_phi_copies(gen, current_region_id, buffer);

		const InstrStorageLocation cond_loc = gen->instr_storage[instr->branch.condition.value];
		assert(cond_loc.kind == INSTR_STORAGE_REG);

		encode_2(buffer,
				MNEMONIC_TEST,
				operand_reg(cond_loc.reg, 64),
				operand_reg(cond_loc.reg, 64));

		_x64_generate_code(gen, instr->branch.true_region, NULL);
		_x64_generate_code(gen, instr->branch.false_region, NULL);
		return;
	}
	case INSTR_JUMP: {
		// HACK: Need to store somewhere the currently processed region
		uint16_t current_region_id = (uint16_t)(buffer - gen->per_region_code_buffer);

		_x64_generate_code(gen, instr->jump.io_state, buffer);

		_x64_generate_phi_variants(gen, current_region_id, buffer);

		_x64_generate_phi_copies(gen, current_region_id, buffer);

		_x64_generate_code(gen, instr->jump.target_region, NULL);
		return;
	}

	case INSTR_RETURN_VALUE:
		_x64_generate_code(gen, instr->return_value.io_state, buffer);
		_x64_generate_code(gen, instr->return_value.value, buffer);
		const InstrStorageLocation return_value_loc = gen->instr_storage[instr->return_value.value.value];
		assert(return_value_loc.kind == INSTR_STORAGE_REG);

		_emit_mov_regs(buffer, return_value_loc.reg, X64_REG_A, 64);

		_emit_return(buffer);
		return;
	case INSTR_RET:
		_x64_generate_code(gen, instr->ret.io_state, buffer);
		_emit_return(buffer);
		return;
	
	case INSTR_IO_STATE:
		if (instr->io_state.producer.value != UINT16_MAX) {
			_x64_generate_code(gen, instr->io_state.producer, buffer);
		}

		return;
	
	case INSTR_CALL_INTERNAL: {
		assert(instr_storage.kind == INSTR_STORAGE_REG);

		_x64_generate_code(gen, instr->call_internal.io_state, buffer);

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
		if (args.count == 1) {
			InstrIndex arg_instr = gen->instr_buffer.inputs_buffer[args.start + 0];
			_x64_generate_code(gen, arg_instr, buffer);
		}

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

		// call
		uint8_t* instr_bytes = code_buffer_append(buffer, 2);
		instr_bytes[0] = 0xff;
		instr_bytes[1] = _mod_rm_with_ext(2, 0);

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

	case INSTR_REGION: {
		CodeBuffer* code_buffer = &gen->per_region_code_buffer[instr->region.id];
		code_buffer_init(code_buffer, gen->allocator);
		_x64_generate_code(gen, instr->region.last_instr, code_buffer);
		return;
	}
	case INSTR_PHI:
		// Nothing to do here, everything is already handled during code gen of `INSTR_REGION`

		InstrInputs variants = instr->phi.variants;
		for (uint16_t i = 0; i < variants.count; i += 1) {
			InstrIndex instr = gen->instr_buffer.inputs_buffer[variants.start + i];
			_x64_generate_code(gen, instr, buffer);
		}

		return;
	case INSTR_SELECT: {
		return;
	}
	}

	unreachable();
}

static size_t _compute_control_instr_encoding_size(const Instr* instr) {
	size_t jump_offset_size = sizeof(uint32_t);

	switch (instr->kind) {
	case INSTR_JUMP:
		return compute_encoding_size_1(MNEMONIC_JMP, operand_rel32(0));
	case INSTR_BRANCH:
		// A branch gets encoded as two jumps:
		// 1. Jump to the true region if condition is true
		// 2. Jump to the false region otherwise
		return jump_offset_size + 2 + jump_offset_size + 1;
	case INSTR_RETURN_VALUE:
	case INSTR_RET:
		return 1;
	default:
		unreachable();
	}

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

		encode_1(buffer, MNEMONIC_JNZ, operand_rel32((int32_t)true_relative_offset));
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

MachineCodeBuffer x64_generate_code(X64CodeGenerator* gen, InstrIndex root_region) {
	encoding_init();

	bool validation_result = _x64_validate(gen);
	assert(validation_result);

	ArenaRegion temp = arena_begin_temp(gen->temp_allocator);

	InstrIndexArray regions_in_dfs_order = _instr_gather_regions_in_dfs_order(gen->instr_buffer,
			gen->allocator,
			gen->temp_allocator,
			root_region);

	uint16_t region_count = gen->instr_buffer.region_count;
	gen->per_region_code_buffer = arena_alloc_array_zeroed(gen->temp_allocator,
			CodeBuffer,
			region_count);

	_x64_generate_code(gen, root_region, NULL);

	uint16_t* blocks_in_dfs_order = arena_alloc_array(gen->allocator, uint16_t, regions_in_dfs_order.count);
	for (size_t i = 0; i < regions_in_dfs_order.count; i += 1) {
		InstrBuffer* instr_buffer = &gen->instr_buffer;
		const Instr* instr = instr_buffer_at(instr_buffer, regions_in_dfs_order.instr[i]);
		blocks_in_dfs_order[i] = instr->region.id;
	}

	size_t final_code_size = 0;
	size_t* code_block_offsets = arena_alloc_array(gen->temp_allocator, size_t, region_count);
	size_t* control_instr_size = arena_alloc_array(gen->temp_allocator, size_t, region_count);
	for (uint16_t i = 0; i < regions_in_dfs_order.count; i += 1) {
		InstrBuffer* instr_buffer = &gen->instr_buffer;

		const Instr* region_instr = instr_buffer_at(instr_buffer, regions_in_dfs_order.instr[i]);
		uint16_t region_id = instr_region_id(&gen->instr_buffer, regions_in_dfs_order.instr[i]);

		const CodeBuffer* code_buffer = &gen->per_region_code_buffer[region_id];
		code_block_offsets[region_id] = final_code_size;
		control_instr_size[region_id] = _compute_control_instr_encoding_size(
				instr_buffer_at(instr_buffer, region_instr->region.last_instr));

		final_code_size += code_buffer->size;
		final_code_size += control_instr_size[region_id];
	}

	void* executable_memory = allocate_executable(final_code_size);
	for (uint16_t i = 0; i < regions_in_dfs_order.count; i += 1) {
		InstrBuffer* instr_buffer = &gen->instr_buffer;

		const Instr* region_instr = instr_buffer_at(instr_buffer, regions_in_dfs_order.instr[i]);
		uint16_t region_id = instr_region_id(&gen->instr_buffer, regions_in_dfs_order.instr[i]);

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
	return machine_code;
}
