#ifndef X64_ENCODING_H
#define X64_ENCODING_H

#include "core/core.h"

//
// CodeBuffer
//

typedef struct {
	uint8_t* buffer;
	size_t size;
	size_t capacity;
	Arena* allocator;
} CodeBuffer;

void code_buffer_init(CodeBuffer* buffer, Arena* allocator);
void code_buffer_grow(CodeBuffer* buffer, size_t expected_capacity);

inline uint8_t* code_buffer_append(CodeBuffer* buffer, size_t byte_count) {
	if (buffer->size + byte_count > buffer->capacity) {
		code_buffer_grow(buffer, buffer->size + byte_count);
	}

	uint8_t* bytes = buffer->buffer + buffer->size;
	buffer->size += byte_count;
	return bytes;
}

inline void code_buffer_push_64(CodeBuffer* buffer, uint64_t value) {
	uint8_t* a = code_buffer_append(buffer, sizeof(value));
	a[7] = (uint8_t)(value >> 56);
	a[6] = (uint8_t)((value >> 48) & 0xff);
	a[5] = (uint8_t)((value >> 40) & 0xff);
	a[4] = (uint8_t)((value >> 32) & 0xff);
	a[3] = (uint8_t)((value >> 24) & 0xff);
	a[2] = (uint8_t)((value >> 16) & 0xff);
	a[1] = (uint8_t)((value >> 8) & 0xff);
	a[0] = (uint8_t)((value >> 0) & 0xff);
}

inline void code_buffer_push_32(CodeBuffer* buffer, uint32_t value) {
	uint8_t* a = code_buffer_append(buffer, sizeof(value));
	a[3] = (uint8_t)((value >> 24) & 0xff);
	a[2] = (uint8_t)((value >> 16) & 0xff);
	a[1] = (uint8_t)((value >> 8) & 0xff);
	a[0] = (uint8_t)((value >> 0) & 0xff);
}

inline void code_buffer_push_8(CodeBuffer* buffer, uint8_t value) {
	uint8_t* a = code_buffer_append(buffer, sizeof(value));
	*a = value;
}

//
// Encoding
//

typedef struct Operand Operand;

typedef enum {
	MNEMONIC_ADD,
	MNEMONIC_SUB,

	MNEMONIC_CMP,

	MNEMONIC_PUSH,
	MNEMONIC_POP,

	MNEMONIC_TEST,

	MNEMONIC_SETZ,
	MNEMONIC_SETNZ,
	MNEMONIC_SETBE,
	MNEMONIC_SETNBE,
	MNEMONIC_SETS,
	MNEMONIC_SETNS,
	MNEMONIC_SETP,
	MNEMONIC_SETNP,
	MNEMONIC_SETL,
	MNEMONIC_SETNL,
	MNEMONIC_SETLE,
	MNEMONIC_SETNLE,

	MNEMONIC_MOV,

	MNEMONIC_MOVZX,

	MNEMONIC_SHL,

	MNEMONIC_COUNT,
} MnemonicKind;

enum OperandKind {
	OP_NONE  = 0,
	OP_REG   = 1 << 0,
	OP_IMM   = 1 << 1,
	OP_MEM   = 1 << 2,
};

typedef uint8_t OperandKind;

struct Operand {
	OperandKind kind;
	uint8_t bit_count;

	union {
		uint8_t reg;
		uint64_t imm;
	};
};

inline Operand operand_none() { return (Operand) {}; }

inline Operand operand_reg(uint8_t reg_index, uint8_t bit_count) {
	Operand op = {};
	op.kind = OP_REG;
	op.reg = reg_index;
	op.bit_count = bit_count;
	return op;
}

inline Operand operand_mem(uint8_t reg_index, uint8_t bit_count) {
	Operand op = {};
	op.kind = OP_MEM;
	op.reg = reg_index;
	op.bit_count = bit_count;
	return op;
}

inline Operand operand_imm(uint64_t imm, uint8_t bit_count) {
	Operand op = {};
	op.kind = OP_IMM;
	op.imm = imm;
	op.bit_count = bit_count;
	return op;
}

// Initialize look up tables need to accelerate encoding
void encoding_init();

// For instructoons that store a result, the first register is the destination
void encode_n(CodeBuffer* code_buffer,
		MnemonicKind mnemonic,
		const Operand* operands,
		uint8_t operand_count);

inline void encode_1(CodeBuffer* code_buffer,
		MnemonicKind mnemonic,
		Operand op0) {
	Operand operands[] = { op0 };
	encode_n(code_buffer, mnemonic, operands, 1);
}

inline void encode_2(CodeBuffer* code_buffer,
		MnemonicKind mnemonic,
		Operand op0,
		Operand op1) {
	Operand operands[] = { op0, op1 };
	encode_n(code_buffer, mnemonic, operands, 2);
}

#endif
