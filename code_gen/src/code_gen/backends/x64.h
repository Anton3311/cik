#ifndef X64_H
#define X64_H

#include "code_gen/instr.h"

#define REG_INDEX_MASK_BIT_COUNT (4)
#define REG_INDEX_MASK ((1 << REG_INDEX_MASK_BIT_COUNT) - 1)

typedef enum {
	REG_FLAG_8_BIT       = (1 << 0) << REG_INDEX_MASK_BIT_COUNT,
	REG_FLAG_16_BIT      = (1 << 1) << REG_INDEX_MASK_BIT_COUNT,
	REG_FLAG_32_BIT      = (1 << 2) << REG_INDEX_MASK_BIT_COUNT,
	REG_FLAG_64_BIT      = (1 << 3) << REG_INDEX_MASK_BIT_COUNT,
} X64RegisterFlag;

typedef enum {
	REG_A,
	REG_C,
	REG_D,
	REG_B,

	REG_SP,
	REG_BP,
	REG_SI,
	REG_DI,

	REG_8,
	REG_9,
	REG_10,
	REG_11,
	REG_12,
	REG_13,
	REG_14,
	REG_15,
} X64Register;

// `size_index` - one of [0, 1, 2, 3]
#define make_reg_id(base_reg_name, size_index) ((base_reg_name) | ((1 << size_index) << REG_INDEX_MASK_BIT_COUNT))

typedef enum {
	INSTR_STORAGE_NONE,
	INSTR_STORAGE_REG,
	INSTR_STORAGE_STACK,
} InstrStorageKind;

typedef struct {
	uint16_t allowed_registers;
	uint8_t reg_size;
} X64InstrStorageRequirement;

typedef struct {
	InstrStorageKind kind;
	union {
		X64Register reg;
	};
} InstrStorageLocation;

typedef struct {
	InstrBuffer instr_buffer;
	InstrUsageRange* usage_ranges;
	InstrStorageLocation* instr_storage;

	Arena* allocator;
	Arena* temp_allocator;
} X64CodeGenerator;

typedef struct {
	void* code;
	size_t size_in_bytes;
} MachineCodeBuffer;

void x64_alloc_registers(X64CodeGenerator* gen, uint16_t allowed_registers);
MachineCodeBuffer x64_generate_code(X64CodeGenerator* gen, InstrIndex root_region);

#endif
