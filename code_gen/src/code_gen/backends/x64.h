#ifndef X64_H
#define X64_H

#include "code_gen/instr.h"
#include "code_gen/code_gen.h"
#include "code_gen/backends/x64_encoding.h"

typedef enum {
	X64_REG_A,
	X64_REG_C,
	X64_REG_D,
	X64_REG_B,

	X64_REG_SP,
	X64_REG_BP,
	X64_REG_SI,
	X64_REG_DI,

	X64_REG_8,
	X64_REG_9,
	X64_REG_10,
	X64_REG_11,
	X64_REG_12,
	X64_REG_13,
	X64_REG_14,
	X64_REG_15,
} X64Register;

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

	CodeBuffer* per_region_code_buffer;

	const FunctionRefTable* ref_table;

	uint16_t* phi_variant_counts_per_region;
	InstrIndexArray* phi_variants_per_region;

	// A per region array of phi instructions that select a variant from that region.
	// Size of the array is in `phi_variant_counts_per_region`
	InstrIndex** phi_node_of_variant;

	StringArray string_consts;

	// Used to map from string constant id to an offset in the `merged_strings_buffer`.
	// Allocated using the `temp_allocator`, thus not usable after code generation finishes.
	size_t* string_offsets;

	// All the string constants used in the source code, are turned into null-terminated strings
	// and then stored sequentionally in this buffer.
	char* merged_strings_buffer;

	uint16_t current_linearized_region_id;
} X64CodeGenerator;

typedef struct {
	void* code;
	size_t size_in_bytes;
} MachineCodeBuffer;

MachineCodeBuffer x64_generate_code(X64CodeGenerator* gen, InstrIndex root_region);

#endif
