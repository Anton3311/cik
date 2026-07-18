#ifndef X64_REG_ALLOC
#define X64_REG_ALLOC

#include "code_gen/backends/x64.h"

typedef struct {
	InstrStorageLocation* allocations;
	InstrIndexArray instr_with_storage_requirement;
	UInt16Array* interference_graph;
} RegisterAllocationResult;

RegisterAllocationResult x64_alloc_regs(const InstrBuffer* instr_buffer,
		InstrLiveRange* live_ranges,
		uint16_t allowed_registers,
		Arena* allocator,
		Arena* temp_allocator);

#endif
