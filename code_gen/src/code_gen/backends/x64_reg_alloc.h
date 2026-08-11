#ifndef X64_REG_ALLOC
#define X64_REG_ALLOC

#include "code_gen/backends/x64.h"
#include "code_gen/abi.h"

typedef struct {
	InstrStorageLocation* allocations;
	InstrIndexArray* interference_graph;

	uint32_t stack_usage;
} RegisterAllocationResult;

RegisterAllocationResult x64_alloc_regs(const InstrBuffer* instr_buffer,
		InstrLiveRange* live_ranges,
		uint16_t allowed_registers,
		const AbiSignature* current_function_signature,
		const AbiSignature* function_signatures,
		Arena* allocator,
		Arena* temp_allocator);

#endif
