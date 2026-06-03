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
	MnemonicKind mnemonic;
	EncodingFlags flags;
	uint8_t opcode;
	uint8_t mod_rm_ext;

	OperandKind op0_kind;
	uint8_t op0_sizes;
	OperandKind op1_kind;
	uint8_t op1_sizes;
} Encoding;

static Encoding s_encodings[] = {
	// add
	(Encoding) { MNEMONIC_ADD, ENC_NONE, 0x00, 0x0, OP_REG | OP_MEM, 8,            OP_REG,          8 },
	(Encoding) { MNEMONIC_ADD, ENC_NONE, 0x01, 0x0, OP_REG | OP_MEM, 16 | 32 | 64, OP_REG,          16 | 32 | 64 },
	(Encoding) { MNEMONIC_ADD, ENC_NONE, 0x02, 0x0, OP_REG,          8,            OP_REG | OP_MEM, 8 },
	(Encoding) { MNEMONIC_ADD, ENC_NONE, 0x03, 0x0, OP_REG,          16 | 32 | 64, OP_REG | OP_MEM, 16 | 32 | 64 },

	(Encoding) { MNEMONIC_ADD, ENC_NONE, 0x81, 0x0, OP_REG | OP_MEM, 16 | 32 | 64, OP_IMM, 16 | 32 },

	// sub
	(Encoding) { MNEMONIC_SUB, ENC_NONE, 0x28, 0x0, OP_REG | OP_MEM, 8,            OP_REG,          8 },
	(Encoding) { MNEMONIC_SUB, ENC_NONE, 0x29, 0x0, OP_REG | OP_MEM, 16 | 32 | 64, OP_REG,          16 | 32 | 64 },
	(Encoding) { MNEMONIC_SUB, ENC_NONE, 0x2a, 0x0, OP_REG,          8,            OP_REG | OP_MEM, 8 },
	(Encoding) { MNEMONIC_SUB, ENC_NONE, 0x2b, 0x0, OP_REG,          16 | 32 | 64, OP_REG | OP_MEM, 16 | 32 | 64 },

	(Encoding) { MNEMONIC_SUB, ENC_NONE, 0x81, 0x5, OP_REG | OP_MEM, 16 | 32 | 64, OP_IMM, 16 | 32 },

	// cmp
	(Encoding) { MNEMONIC_CMP, ENC_NONE, 0x38, 0x0, OP_REG | OP_MEM, 8,            OP_REG,          8 },
	(Encoding) { MNEMONIC_CMP, ENC_NONE, 0x39, 0x0, OP_REG | OP_MEM, 16 | 32 | 64, OP_REG,          16 | 32 | 64 },
	(Encoding) { MNEMONIC_CMP, ENC_NONE, 0x3a, 0x0, OP_REG,          8,            OP_REG | OP_MEM, 8 },
	(Encoding) { MNEMONIC_CMP, ENC_NONE, 0x3b, 0x0, OP_REG,          16 | 32 | 64, OP_REG | OP_MEM, 16 | 32 | 64 },

	// test
	(Encoding) { MNEMONIC_TEST, ENC_NONE, 0x84, 0x0, OP_REG | OP_MEM, 8,            OP_REG, 8 },
	(Encoding) { MNEMONIC_TEST, ENC_NONE, 0x84, 0x0, OP_REG | OP_MEM, 16 | 32 | 64, OP_REG, 16 | 32 | 64 },

	// mov
	(Encoding) { MNEMONIC_MOV, ENC_NONE, 0x88, 0x0, OP_REG | OP_MEM, 8,            OP_REG, 8 },
	(Encoding) { MNEMONIC_MOV, ENC_NONE, 0x89, 0x0, OP_REG | OP_MEM, 16 | 32 | 64, OP_REG, 16 | 32 | 64 },

	(Encoding) { MNEMONIC_MOV, ENC_ADD_REG_TO_OPCODE, 0xb0, 0x0, OP_REG, 8,            OP_IMM, 8},
	(Encoding) { MNEMONIC_MOV, ENC_ADD_REG_TO_OPCODE, 0xb8, 0x0, OP_REG, 16 | 32 | 64, OP_IMM, 16 | 32 | 64},

	// shl
	(Encoding) { MNEMONIC_SHL, ENC_NONE, 0xc1, 0x4, OP_REG | OP_MEM, 16 | 32 | 64, OP_IMM, 8 },
};

void encode(CodeBuffer* code_buffer, MnemonicKind mnemonic, Operand op0, Operand op1) {
	profile_scope_start(__func__);

	bool encoded = false;
	for (size_t i = 0; i < array_size(s_encodings); i += 1) {
		if (s_encodings[i].mnemonic != mnemonic) {
			continue;
		}

		Encoding encoding = s_encodings[i];
		bool supports_operands = has_flag(encoding.op0_kind, op0.kind)
			&& has_flag(encoding.op1_kind, op1.kind);
		bool supports_sizes = has_flag(encoding.op0_sizes, op0.bit_count)
			&& has_flag(encoding.op1_sizes, op1.bit_count);

		if (!supports_operands || !supports_sizes) {
			continue;
		}

		assert_msg(op0.kind != OP_MEM, "Not yet implemented");
		assert_msg(op1.kind != OP_MEM, "Not yet implemented");

		uint8_t rex_prefix_bits = 0;
		uint8_t mod_rm_byte = MOD_RM_RM;

		if (op1.kind == OP_REG) {
			rex_prefix_bits |= ((op1.reg >> 3) << 2);
			mod_rm_byte |= (op1.reg & 0b111) << 3;

			if (op1.bit_count == 64) {
				rex_prefix_bits |= 0b1000;
			}
		} else if (op1.kind == OP_IMM) {
			// the input is an imm, so set use the mod rm extension
			mod_rm_byte |= (encoding.mod_rm_ext << 3);
		}

		if (op0.kind == OP_REG) {
			rex_prefix_bits |= ((op0.reg >> 3) << 0);

			if (!has_flag(encoding.flags, ENC_ADD_REG_TO_OPCODE)) {
				mod_rm_byte |= (op0.reg & 0b111) << 0;
			}

			if (op0.bit_count == 64) {
				rex_prefix_bits |= 0b1000;
			}
		}

		size_t encoding_size = 0;
		// 0f prefix
		encoding_size += has_flag(encoding.flags, ENC_HAS_0F_PREFIX) ? 1 : 0;
		// rex prefix
		encoding_size += (rex_prefix_bits == 0) ? 0 : 1;
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

		// 0f prefix
		if (has_flag(encoding.flags, ENC_HAS_0F_PREFIX)) {
			*write_ptr = 0x0f;
			write_ptr += 1;
		}

		// rex prefix
		if (rex_prefix_bits) {
			*write_ptr = 0b01000000 | rex_prefix_bits;
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
			*write_ptr = mod_rm_byte;
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
