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

#define X64_REG_COUNT 16

typedef enum {
	INSTR_STORAGE_NONE,
	INSTR_STORAGE_REG,
	INSTR_STORAGE_STACK,
} InstrStorageKind;

typedef enum {
	X64_NONE                       = 0,
	X64_PRINT_SCHEDULED_IR         = 1 << 0,
	X64_DEBUG_LOG                  = 1 << 1,
	X64_PRINT_ASSIGNED_STORAGE_LOC = 1 << 2,
	X64_SKIP_REG_ALLOC             = 1 << 3,
} X64BackendFlags;

typedef struct {
	uint16_t allowed_registers;
	uint8_t reg_size;
} X64InstrStorageRequirement;

typedef struct {
	InstrStorageKind kind;
	union {
		X64Register reg;
		struct {
			uint32_t offset;
		} stack;
	};
} InstrStorageLocation;

typedef enum {
	CALL_ADDR_ABSOLUTE,
	CALL_ADDR_RELATIVE,
} CallAddressKind;

typedef struct {
	// At which byte offset does, the instruction requiring the function address end.
	//
	// This is required, because calls use relative offsets from the end of the call instruction.
	size_t instruction_end_offset;

	// At which offset in the code, does the address value appear.
	size_t addr_offset;

	CallAddressKind kind;

	uint8_t function_index;
} CallAddressPlaceholder;

typedef struct {
	X64BackendFlags flags;

	InstrBuffer instr_buffer;
	InstrLiveRange* live_ranges;
	InstrStorageLocation* instr_storage;

	Arena* allocator;
	Arena* temp_allocator;

	CodeBuffer* per_region_code_buffer;

	// Temporary structures
	InstrIndexArray instr_with_storage_requirement;

	// Produced by the register allocator.
	//
	// For each instruction in `instr_with_storage_requirement` stores an array instructions that
	// interfere with it.
	UInt16Array* interference_graph;

	uint16_t* phi_variant_counts_per_region;
	InstrIndexArray* phi_variants_per_region;

	// A per region array of phi instructions that select a variant from that region.
	// Size of the array is in `phi_variant_counts_per_region`
	InstrIndex** phi_node_of_variant;

	// Precomputed sizes (bit-count) for phi instructions.
	uint8_t* phi_sizes;

	StringArray string_consts;

	// Used to map from string constant id to an offset in the `merged_strings_buffer`.
	// Allocated using the `temp_allocator`, thus not usable after code generation finishes.
	size_t* string_offsets;

	// All the string constants used in the source code, are turned into null-terminated strings
	// and then stored sequentionally in this buffer.
	char* merged_strings_buffer;

	CallAddressPlaceholder* call_addr_placeholders;
	size_t call_addr_placeholder_count;
	size_t call_addr_placeholder_capacity;

	// Stores the region id, where this palceholder was generated. Later used to transform the
	// offsets from code buffer/region local to global offsets (offsets in the buffer where all the
	// code from all regions is merged together).
	uint16_t* call_addr_placeholder_regions;

	uint32_t stack_usage;
} X64CodeGenerator;

typedef struct {
	void* code;
	size_t size_in_bytes;
} MachineCodeBuffer;

typedef struct {
	const void* code;
	size_t size_in_bytes;

	CallAddressPlaceholder* call_addr_placeholders;
	size_t call_addr_placeholder_count;
} LoweredFunction;

LoweredFunction x64_generate_code(X64CodeGenerator* gen, InstrIndex root_region);

//
// Internal
//

typedef struct {
	X64Register src;
	X64Register dst;
} RegisterMove;

typedef struct {
	RegisterMove* moves;
	size_t count;
} RegisterMoveArray;

RegisterMoveArray _parallel_move_values(
		const InstrStorageLocation* input_instr_storage,
		const X64Register* expected_locs,
		size_t expected_loc_count,
		uint16_t allowed_temp_register,
		Arena* allocator,
		Arena* temp_allocator);

#endif
