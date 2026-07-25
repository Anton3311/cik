#ifndef X64_LINKER
#define X64_LINKER

#include "code_gen/backends/x64.h"
#include "code_gen/code_gen.h"

typedef struct {
	MachineCodeBuffer machine_code;
	size_t* function_offsets;
} LinkedProgram;

LinkedProgram linker_link(const LoweredFunction* functions,
		size_t function_count,
		const FunctionRefTable* ref_table,
		Arena* allocator);

#endif
