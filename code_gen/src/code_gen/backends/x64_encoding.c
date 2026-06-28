#include "x64_encoding.h"

#include "core/profiler.h"

//
// CodeBuffer
//

static const size_t CODE_BUFFER_ALLOCATION_STEP = 64;
static const size_t CODE_BUFFER_ALLOCATION_STEP_MASK = CODE_BUFFER_ALLOCATION_STEP - 1;

void code_buffer_init(CodeBuffer* buffer, Arena* allocator) {
	buffer->allocator = allocator;
	buffer->capacity = 0;
	buffer->size = 0;
	buffer->buffer = arena_alloc_array(allocator, uint8_t, 0);
}

void code_buffer_wrap(CodeBuffer* buffer, uint8_t* backing_buffer, size_t backing_buffer_size) {
	buffer->allocator = NULL;
	buffer->size = 0;
	buffer->capacity = backing_buffer_size;
	buffer->buffer = backing_buffer;
}

void code_buffer_grow(CodeBuffer* buffer, size_t expected_capacity) {
	assert(buffer->allocator);

	size_t capacity_delta = expected_capacity - buffer->capacity;
	size_t allocation_size = (capacity_delta + CODE_BUFFER_ALLOCATION_STEP - 1) & ~CODE_BUFFER_ALLOCATION_STEP_MASK;

	assert_msg(buffer->buffer + buffer->capacity == buffer->allocator->base + buffer->allocator->allocated,
			"Trying to grow CodeBuffer, however since the last grow there were allocations done, "
			"with the arena associated with this code buffer");

	arena_alloc_array(buffer->allocator, uint8_t, allocation_size);

	buffer->capacity += allocation_size;
}

//
// Encoding
//

enum EncodingFlags {
	ENC_NONE               = 0,
	ENC_HAS_0F_PREFIX      = 1 << 0,

	// The destination reg index is added to the opcode byte. ModRM not used.
	ENC_ADD_REG_TO_OPCODE  = 1 << 1,
};

typedef uint8_t EncodingFlags;

enum ModRM {
	MOD_RM_ADDRESS_RM         = 0b00000000,
	MOD_RM_ADDRESS_RM_DISP_8  = 0b00000000,
	MOD_RM_ADDRESS_RM_DISP_32 = 0b00000000,
	MOD_RM_RM                 = 0b11000000,
};

#define MAX_OPERAND_COUNT 4

typedef uint8_t ModRM;

typedef struct {
	OperandKind kind;
	uint8_t sizes;
} OperandEncoding;

typedef struct {
	MnemonicKind mnemonic;
	EncodingFlags flags;
	uint8_t opcode;
	uint8_t mod_rm_ext;

	OperandEncoding operands[MAX_OPERAND_COUNT];
} Encoding;

typedef struct {
	size_t start;
	size_t end;
} EncodingRange;

typedef struct {
	uint8_t operand_count;
	uint8_t reg_operand_count;
	bool has_mod_rm;
} EncodingExtra;

#define OP_RM OP_REG | OP_MEM

static Encoding s_encodings[] = {
	// add
	{ MNEMONIC_ADD, ENC_NONE, 0x00, 0x0, { { OP_RM,  8 },            { OP_REG, 8 } } },
	{ MNEMONIC_ADD, ENC_NONE, 0x01, 0x0, { { OP_RM,  16 | 32 | 64 }, { OP_REG, 16 | 32 | 64 } } },
	{ MNEMONIC_ADD, ENC_NONE, 0x02, 0x0, { { OP_REG, 8 },            { OP_RM,  8 } } },
	{ MNEMONIC_ADD, ENC_NONE, 0x03, 0x0, { { OP_REG, 16 | 32 | 64 }, { OP_RM,  16 | 32 | 64 } } },

	{ MNEMONIC_ADD, ENC_NONE, 0x81, 0x0, { { OP_RM, 16 | 32 | 64 }, { OP_IMM, 16 | 32 } } },

	// sub
	{ MNEMONIC_SUB, ENC_NONE, 0x28, 0x0, { { OP_RM,  8 },            { OP_REG, 8 } } },
	{ MNEMONIC_SUB, ENC_NONE, 0x29, 0x0, { { OP_RM,  16 | 32 | 64 }, { OP_REG, 16 | 32 | 64 } } },
	{ MNEMONIC_SUB, ENC_NONE, 0x2a, 0x0, { { OP_REG, 8 },            { OP_RM,  8 } } },
	{ MNEMONIC_SUB, ENC_NONE, 0x2b, 0x0, { { OP_REG, 16 | 32 | 64 }, { OP_RM,  16 | 32 | 64 } } },

	{ MNEMONIC_SUB, ENC_NONE, 0x81, 0x5, { { OP_RM, 16 | 32 | 64 }, { OP_IMM, 16 | 32 } } },

	// cmp
	{ MNEMONIC_CMP, ENC_NONE, 0x38, 0x0, { { OP_RM,  8 },            { OP_REG, 8 } } },
	{ MNEMONIC_CMP, ENC_NONE, 0x39, 0x0, { { OP_RM,  16 | 32 | 64 }, { OP_REG, 16 | 32 | 64 } } },
	{ MNEMONIC_CMP, ENC_NONE, 0x3a, 0x0, { { OP_REG, 8 },            { OP_RM,  8 } } },
	{ MNEMONIC_CMP, ENC_NONE, 0x3b, 0x0, { { OP_REG, 16 | 32 | 64 }, { OP_RM,  16 | 32 | 64 } } },

	// push
	{ MNEMONIC_PUSH, ENC_ADD_REG_TO_OPCODE, 0x50, 0x0, { { OP_RM, 16 | 64 } } },

	// pop
	{ MNEMONIC_POP,  ENC_ADD_REG_TO_OPCODE, 0x58, 0x0, { { OP_RM, 16 | 64 } } },

	// j[0,b,nb,z,nz,be,nbe,s,ns,p,np,l,nl,le,nle]
	{ MNEMONIC_JO,   ENC_HAS_0F_PREFIX, 0x80, 0x0, { { OP_REL, 16 | 32 } } },
	{ MNEMONIC_JNO,  ENC_HAS_0F_PREFIX, 0x81, 0x0, { { OP_REL, 16 | 32 } } },
	{ MNEMONIC_JB,   ENC_HAS_0F_PREFIX, 0x82, 0x0, { { OP_REL, 16 | 32 } } },
	{ MNEMONIC_JNB,  ENC_HAS_0F_PREFIX, 0x83, 0x0, { { OP_REL, 16 | 32 } } },
	{ MNEMONIC_JZ,   ENC_HAS_0F_PREFIX, 0x84, 0x0, { { OP_REL, 16 | 32 } } },
	{ MNEMONIC_JNZ,  ENC_HAS_0F_PREFIX, 0x85, 0x0, { { OP_REL, 16 | 32 } } },
	{ MNEMONIC_JBE,  ENC_HAS_0F_PREFIX, 0x86, 0x0, { { OP_REL, 16 | 32 } } },
	{ MNEMONIC_JNBE, ENC_HAS_0F_PREFIX, 0x87, 0x0, { { OP_REL, 16 | 32 } } },
	{ MNEMONIC_JS,   ENC_HAS_0F_PREFIX, 0x88, 0x0, { { OP_REL, 16 | 32 } } },
	{ MNEMONIC_JNS,  ENC_HAS_0F_PREFIX, 0x89, 0x0, { { OP_REL, 16 | 32 } } },
	{ MNEMONIC_JP,   ENC_HAS_0F_PREFIX, 0x8a, 0x0, { { OP_REL, 16 | 32 } } },
	{ MNEMONIC_JNP,  ENC_HAS_0F_PREFIX, 0x8b, 0x0, { { OP_REL, 16 | 32 } } },
	{ MNEMONIC_JL,   ENC_HAS_0F_PREFIX, 0x8c, 0x0, { { OP_REL, 16 | 32 } } },
	{ MNEMONIC_JNL,  ENC_HAS_0F_PREFIX, 0x8d, 0x0, { { OP_REL, 16 | 32 } } },
	{ MNEMONIC_JLE,  ENC_HAS_0F_PREFIX, 0x8e, 0x0, { { OP_REL, 16 | 32 } } },
	{ MNEMONIC_JNLE, ENC_HAS_0F_PREFIX, 0x8f, 0x0, { { OP_REL, 16 | 32 } } },

	// test
	{ MNEMONIC_TEST, ENC_NONE, 0x84, 0x0, { { OP_RM, 8 },            { OP_REG, 8 } } },
	{ MNEMONIC_TEST, ENC_NONE, 0x84, 0x0, { { OP_RM, 16 | 32 | 64 }, { OP_REG, 16 | 32 | 64 } } },

	// set[z,nz,be,nbe,s,ns,p,no,l,nl,nle]
	{ MNEMONIC_SETZ,   ENC_HAS_0F_PREFIX, 0x94, 0x0, { { OP_RM, 8 } } }, 
	{ MNEMONIC_SETNZ,  ENC_HAS_0F_PREFIX, 0x95, 0x0, { { OP_RM, 8 } } },
	{ MNEMONIC_SETBE,  ENC_HAS_0F_PREFIX, 0x96, 0x0, { { OP_RM, 8 } } },
	{ MNEMONIC_SETNBE, ENC_HAS_0F_PREFIX, 0x97, 0x0, { { OP_RM, 8 } } },
	{ MNEMONIC_SETS,   ENC_HAS_0F_PREFIX, 0x98, 0x0, { { OP_RM, 8 } } },
	{ MNEMONIC_SETNS,  ENC_HAS_0F_PREFIX, 0x99, 0x0, { { OP_RM, 8 } } },
	{ MNEMONIC_SETP,   ENC_HAS_0F_PREFIX, 0x9a, 0x0, { { OP_RM, 8 } } },
	{ MNEMONIC_SETNP,  ENC_HAS_0F_PREFIX, 0x9b, 0x0, { { OP_RM, 8 } } },
	{ MNEMONIC_SETL,   ENC_HAS_0F_PREFIX, 0x9c, 0x0, { { OP_RM, 8 } } },
	{ MNEMONIC_SETNL,  ENC_HAS_0F_PREFIX, 0x9d, 0x0, { { OP_RM, 8 } } },
	{ MNEMONIC_SETLE,  ENC_HAS_0F_PREFIX, 0x9e, 0x0, { { OP_RM, 8 } } },
	{ MNEMONIC_SETNLE, ENC_HAS_0F_PREFIX, 0x9f, 0x0, { { OP_RM, 8 } } },

	// mov
	{ MNEMONIC_MOV, ENC_NONE, 0x88, 0x0, { { OP_RM,  8 },            { OP_REG, 8 } } },
	{ MNEMONIC_MOV, ENC_NONE, 0x89, 0x0, { { OP_RM,  16 | 32 | 64 }, { OP_REG, 16 | 32 | 64 } } },
	{ MNEMONIC_MOV, ENC_NONE, 0x8a, 0x0, { { OP_REG, 8 },            { OP_RM,  8 } } },
	{ MNEMONIC_MOV, ENC_NONE, 0x8b, 0x0, { { OP_REG, 16 | 32 | 64 }, { OP_RM,  16 | 32 | 64 } } },

	{ MNEMONIC_MOV, ENC_ADD_REG_TO_OPCODE, 0xb0, 0x0, { { OP_REG, 8 },            { OP_IMM, 8 } } },
	{ MNEMONIC_MOV, ENC_ADD_REG_TO_OPCODE, 0xb8, 0x0, { { OP_REG, 16 | 32 | 64 }, { OP_IMM, 16 | 32 | 64 } } },

	// movzx
	{ MNEMONIC_MOVZX, ENC_HAS_0F_PREFIX, 0xb6, 0x0, { { OP_RM, 8  }, { OP_REG, 16 | 32 | 64 } } },
	{ MNEMONIC_MOVZX, ENC_HAS_0F_PREFIX, 0xb7, 0x0, { { OP_RM, 16 }, { OP_REG, 16 | 32 | 64 } } },

	// shl
	{ MNEMONIC_SHL, ENC_NONE, 0xc1, 0x4, { { OP_RM, 16 | 32 | 64 }, { OP_IMM, 8 } } },

	// jmp
	{ MNEMONIC_JMP, ENC_NONE, 0xe9, 0x0, { { OP_REL, 16 | 32 } } },

	// neg
	{ MNEMONIC_NEG, ENC_NONE, 0xf6, 0x3, { { OP_RM, 8 } } },
	{ MNEMONIC_NEG, ENC_NONE, 0xf7, 0x3, { { OP_RM, 16 | 32 | 64 } } },

	// call
	{ MNEMONIC_CALL, ENC_NONE, 0xff, 0x2, { { OP_RM, 16 | 32 } } },
	{ MNEMONIC_CALL, ENC_NONE, 0xff, 0x2, { { OP_RM, 64 } } },
};

static EncodingRange s_encoding_ranges[MNEMONIC_COUNT];
static bool s_encoding_initialized = false;
static EncodingExtra s_encoding_extra[array_size(s_encodings)];

void encoding_init() {
	if (s_encoding_initialized) {
		return;
	}

	profile_scope_start(__func__);

	{
		MnemonicKind current_mnemonic = s_encodings[0].mnemonic;
		s_encoding_ranges[current_mnemonic] = (EncodingRange) {};
		for (size_t i = 1; i < array_size(s_encodings); i += 1) {
			s_encoding_ranges[current_mnemonic].end = i;

			if (s_encodings[i].mnemonic != current_mnemonic) {
				current_mnemonic = s_encodings[i].mnemonic;
				assert_msg(s_encoding_ranges[current_mnemonic].start == 0
						&& s_encoding_ranges[current_mnemonic].end == 0,
						"Different encodings for the same mnemonic must be grouped together and "
						"appear as subsequent array elements");

				s_encoding_ranges[current_mnemonic].start = i;
				s_encoding_ranges[current_mnemonic].end = i;
			}
		}

		s_encoding_ranges[current_mnemonic].end = array_size(s_encodings);
	}

	{
		for (size_t i = 0; i < array_size(s_encodings); i += 1) {
			Encoding encoding = s_encodings[i];

			size_t count = 0;
			size_t reg_op_count = 0;
			for (count = 0; count < MAX_OPERAND_COUNT; count += 1) {
				if (encoding.operands[count].kind == OP_NONE) {
					break;
				} else if (has_any_flag(encoding.operands[count].kind, OP_RM)) {
					reg_op_count += 1;
				}
			}

			s_encoding_extra[i].operand_count = (uint8_t)count;
			s_encoding_extra[i].reg_operand_count = (uint8_t)reg_op_count;

			if (has_flag(encoding.flags, ENC_ADD_REG_TO_OPCODE)) {
				assert_msg(reg_op_count >= 1,
						"Encoding requires adding the register index to the"
						" opcode byte, however it has no operands of register or memory type");

				reg_op_count -= 1;
			}

			if (reg_op_count > 0) {
				s_encoding_extra[i].has_mod_rm = true;
			}
		}
	}

	s_encoding_initialized = true;

	profile_scope_end();
}

typedef struct {
	ModRM mod;
	uint8_t reg;
	uint8_t rm;
} ModRMFields;

static ModRMFields _encode_mod_rm(Encoding encoding, Operand op0, Operand op1) {
	assert_msg(!has_flag(encoding.flags, ENC_ADD_REG_TO_OPCODE),
			"Instructions that encode a register in the opcode byte don't have a ModR/M byte");
	
	Operand operands[2] = { op0, op1 };

	ModRMFields fields = {};
	fields.mod = MOD_RM_RM;

	for (size_t i = 0; i < 2; i += 1) {
		OperandKind op_mask = encoding.operands[i].kind;
		if (op_mask == OP_NONE) {
			continue;
		}

		Operand op = operands[i];

		if (op.kind == OP_MEM) {
			fields.mod = MOD_RM_ADDRESS_RM;
		}

		uint8_t reg = 0;
		switch (op.kind) {
		case OP_REG:
			reg = op.reg;
			break;
		case OP_MEM:
			reg = op.reg;
			break;
		case OP_IMM:
			break;
		}

		if (op_mask == (OP_REG | OP_MEM)) {
			fields.rm = reg;
		} else if (op_mask == OP_REG) {
			fields.reg = reg;
		} else if (op_mask == OP_IMM) {
		} else {
			unreachable();
		}
	}

	return fields;
}

static size_t _select_encoding(MnemonicKind mnemonic,
		const Operand* operands,
		uint8_t operand_count) {
	profile_scope_start(__func__);

	if (!s_encoding_initialized) {
		encoding_init();
	}

	assert(mnemonic <= MNEMONIC_COUNT);

	size_t selected_index = SIZE_MAX;

	EncodingRange range = s_encoding_ranges[mnemonic];
	for (size_t i = range.start; i < range.end; i += 1) {
		if (operand_count != s_encoding_extra[i].operand_count) {
			continue;
		}

		Encoding encoding = s_encodings[i];

		bool is_supported = true;
		for (uint8_t j = 0; j < operand_count; j += 1) {
			if (has_flag(encoding.operands[j].kind, operands[j].kind)
				&& has_flag(encoding.operands[j].sizes, operands[j].bit_count)) {
				continue;
			} else {
				is_supported = false;
				break;
			}
		}

		if (!is_supported) {
			continue;
		}

		selected_index = i;
		break;
	}

	profile_scope_end();
	return selected_index;
}

size_t run_encoding_operation(CodeBuffer* code_buffer,
		MnemonicKind mnemonic,
		const Operand* operands,
		uint8_t operand_count,
		EncodingOperation operation) {

	profile_scope_start(__func__);
	assert(operand_count > 0);
	assert(operand_count <= MAX_OPERAND_COUNT);

	size_t encoding_index = _select_encoding(mnemonic, operands, operand_count);
	if (encoding_index == SIZE_MAX) {
		panic("No encoding found");
	}

	Encoding encoding = s_encodings[encoding_index];
	uint8_t rex_prefix_bits = 0;

	ModRMFields fields = {};
	if (s_encoding_extra[encoding_index].has_mod_rm) {
		if (operand_count == 1) {
			fields = _encode_mod_rm(encoding, operands[0], operand_none());
		} else if (operand_count == 2) {
			fields = _encode_mod_rm(encoding, operands[0], operands[1]);
		} else {
			unreachable();
		}

		if (s_encoding_extra[encoding_index].reg_operand_count == 1) {
			fields.reg = encoding.mod_rm_ext;
		}

		rex_prefix_bits |= ((fields.reg >> 3) << 2);
		rex_prefix_bits |= ((fields.rm >> 3) << 0);
	} else {
		assert(operand_count >= 1);
		
		if (operands[0].kind == OP_REG) {
			if (has_flag(encoding.flags, ENC_ADD_REG_TO_OPCODE)) {
				// For instructions that require adding register number to the opcode byte, store
				// the register extension in the B field of the rex prefix instead of R
				rex_prefix_bits |= (operands[0].reg >> 3); // rm field
			} else {
				rex_prefix_bits |= (operands[0].reg >> 3) << 2; // reg field
			}
		}
	}

	for (size_t i = 0; i < operand_count; i += 1) {
		Operand op = operands[i];
		if (op.bit_count == 64 && (op.kind == OP_REG || op.kind == OP_MEM)) {
			rex_prefix_bits |= 0b1000;
		}
	}

	size_t encoding_size = 0;
	// rex prefix
	encoding_size += (rex_prefix_bits == 0) ? 0 : 1;
	// 0f prefix
	encoding_size += has_flag(encoding.flags, ENC_HAS_0F_PREFIX) ? 1 : 0;
	// opcode byte
	encoding_size += 1;
	// modrm byte
	if (s_encoding_extra[encoding_index].has_mod_rm) {
		encoding_size += 1;
	}

	// imm sizes
	for (size_t i = 0; i < operand_count; i += 1) {
		if (operands[i].kind == OP_IMM) {
			encoding_size += operands[i].bit_count / 8;
		} else if (operands[i].kind == OP_REL) {
			encoding_size += operands[i].bit_count / 8;
		}
	}

	assert(encoding_size <= 16);

	switch (operation) {
	case ENC_OP_SIZE: {
		// NOTE: don't forget to end the profiler scope
		profile_scope_end();
		return encoding_size;
	}
	case ENC_OP_ENCODE: {
		assert(code_buffer != NULL);
		// now fill the bytes

		uint8_t* buffer = code_buffer_append(code_buffer, encoding_size);
		uint8_t* write_ptr = buffer;

		// rex prefix
		if (rex_prefix_bits) {
			*write_ptr = 0b01000000 | rex_prefix_bits;
			write_ptr += 1;
		}

		// 0f prefix
		if (has_flag(encoding.flags, ENC_HAS_0F_PREFIX)) {
			*write_ptr = 0x0f;
			write_ptr += 1;
		}

		// opcode byte
		if (has_flag(encoding.flags, ENC_ADD_REG_TO_OPCODE)) {
			assert(operands[0].kind == OP_REG);
			*write_ptr = encoding.opcode + (operands[0].reg & 0b111);
		} else {
			*write_ptr = encoding.opcode;
		}

		write_ptr += 1;

		// morm byte
		if (s_encoding_extra[encoding_index].has_mod_rm) {
			*write_ptr = ((fields.reg & 0b111)<< 3) | (fields.rm & 0b111) | fields.mod;
			write_ptr += 1;
		}

		// write imm
		for (size_t i = 0; i < operand_count; i += 1) {
			if (operands[i].kind == OP_IMM) {
				memcpy(write_ptr, &operands[i].imm, operands[i].bit_count / 8);
				write_ptr += operands[i].bit_count / 8;
			} else if (operands[i].kind == OP_REL) {
				memcpy(write_ptr, &operands[i].rel, sizeof(operands[i].rel));
				write_ptr += sizeof(operands[i].rel);
			}
		}

		assert(write_ptr - buffer == encoding_size);

		// NOTE: don't forget to end the profiler scope
		profile_scope_end();
		return 0;
	}
	}

	unreachable();
	return 0;
}
