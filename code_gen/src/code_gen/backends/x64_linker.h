#ifndef X64_LINKER
#define X64_LINKER

#include "code_gen/backends/x64.h"
#include "code_gen/code_gen.h"

typedef struct {
	LoweredFunction* functions;
	size_t function_count;
} LoweredUnit;

typedef struct {
	uint64_t* func_offsets;
} LinkedUnitState;

typedef struct {
	MachineCodeBuffer machine_code;

	size_t unit_count;
	LinkedUnitState* unit_states;

	void* entry_point_address;
} LinkedProgram;

bool linker_link(const LoweredUnit* units,
		const SymbolMap* imported_symbol_maps,
		const SymbolMap* exported_symbol_maps,
		const SymbolMap* dynamically_linked_symbols,
		size_t unit_count,
		String entry_point_name,
		Arena* allocator,
		LinkedProgram* out_linked);

#endif
