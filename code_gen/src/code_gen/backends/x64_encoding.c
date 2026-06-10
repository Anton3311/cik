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

void code_buffer_grow(CodeBuffer* buffer, size_t expected_capacity) {
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

	OperandEncoding operands[4];
} Encoding;

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
};

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
			fields.reg = encoding.mod_rm_ext;
		} else {
			unreachable();
		}
	}

	return fields;
}

void encode(CodeBuffer* code_buffer, MnemonicKind mnemonic, Operand op0, Operand op1) {
	profile_scope_start(__func__);

	bool encoded = false;
	for (size_t i = 0; i < array_size(s_encodings); i += 1) {
		if (s_encodings[i].mnemonic != mnemonic) {
			continue;
		}

		Encoding encoding = s_encodings[i];
		bool supports_operands = has_flag(encoding.operands[0].kind, op0.kind)
			&& has_flag(encoding.operands[1].kind, op1.kind);
		bool supports_sizes = has_flag(encoding.operands[0].sizes, op0.bit_count)
			&& has_flag(encoding.operands[1].sizes, op1.bit_count);

		if (!supports_operands || !supports_sizes) {
			continue;
		}

		uint8_t rex_prefix_bits = 0;
		Operand operands[2] = { op0, op1 };

		ModRMFields fields = {};
		if (has_flag(encoding.flags, ENC_ADD_REG_TO_OPCODE)) {
			assert(operands[0].kind == OP_REG);
			rex_prefix_bits |= (op0.reg >> 3) << 2; // reg field
		} else {
			fields = _encode_mod_rm(encoding, op0, op1);

			rex_prefix_bits |= ((fields.reg >> 3) << 2);
			rex_prefix_bits |= ((fields.rm >> 3) << 0);
		}

		for (size_t i = 0; i < array_size(operands); i += 1) {
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
		if (!has_flag(encoding.flags, ENC_ADD_REG_TO_OPCODE)) {
			encoding_size += 1;
		}

		// imm sizes
		if (op0.kind == OP_IMM) {
			encoding_size += op0.bit_count / 8;
		}

		if (op1.kind == OP_IMM) {
			encoding_size += op1.bit_count / 8;
		}

		assert(encoding_size <= 16);

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
			*write_ptr = encoding.opcode + (op0.reg & 0b111);
		} else {
			*write_ptr = encoding.opcode;
		}

		write_ptr += 1;

		// morm byte
		if (!has_flag(encoding.flags, ENC_ADD_REG_TO_OPCODE)) {
			*write_ptr = ((fields.reg & 0b111)<< 3) | (fields.rm & 0b111) | fields.mod;
			write_ptr += 1;
		}

		// write imm
		if (op0.kind == OP_IMM) {
			memcpy(write_ptr, &op0.imm, op0.bit_count / 8);
			write_ptr += op0.bit_count / 8;
		}

		if (op1.kind == OP_IMM) {
			memcpy(write_ptr, &op1.imm, op1.bit_count / 8);
			write_ptr += op1.bit_count / 8;
		}

		assert(write_ptr - buffer == encoding_size);
		
		encoded = true;
		break;
	}

	assert_msg(encoded, "No encoding found");

	profile_scope_end();
}
