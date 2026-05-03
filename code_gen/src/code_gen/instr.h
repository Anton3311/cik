#ifndef INSTR_H
#define INSTR_H

#include "core/core.h"

typedef struct Instr Instr;
typedef struct InstrBuffer InstrBuffer;

typedef enum {
	INSTR_NO_OP,

	INSTR_CONST_8,
	INSTR_CONST_16,
	INSTR_CONST_32,
	INSTR_CONST_64,

	INSTR_REGION,
} InstrKind;

typedef struct {
	uint16_t value;
} InstrIndex;

struct Instr {
	InstrKind kind;

	union {
		union {
			uint8_t u8;
			int8_t i8;
		} const_8;

		union {
			uint16_t u16;
			int16_t i16;
		} const_16;

		union {
			uint32_t u32;
			int32_t i32;
			float f32;
		} const_32;

		union {
			uint64_t u64;
			int64_t i64;
			double f64;
		} const_64;

		struct {
			InstrIndex last_instr;
		} region;
	};
};

struct InstrBuffer {
	Instr* instr;
	size_t count;
};

String instr_name(InstrKind instr_kind);
void instr_print(const Instr* instr);
void instr_print_all(InstrBuffer instr_buffer);

#endif
