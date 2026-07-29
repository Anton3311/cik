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

typedef enum {
	LINKER_ERROR_UNRESOLVED_SYMBOL,
	LINKER_ERROR_UNRESOLVED_ENTRY_POINT,
	LINKER_ERROR_DUPLICATE_SYMBOLS,
} LinkerErrorKind;

typedef struct {
	LinkerErrorKind kind;

	union {
		// Where was is referenced?
		uint32_t unit_index;
		SymbolId symbol_id;
	} unresolved;

	union {
		// Where was the first defintion?
		uint32_t first_unit_index;
		SymbolId first_symbol_id;

		// Where was the second defintion?
		uint32_t second_unit_index;
		SymbolId second_symbol_id;
	} duplicate;

	union {
		String name;
	} entry_point;
} LinkerError;

typedef struct {
	MachineCodeBuffer machine_code;

	size_t unit_count;
	LinkedUnitState* unit_states;

	void* entry_point_address;

	LinkerError* errors;
	size_t error_count;
} LinkedProgram;

bool linker_link(const LoweredUnit* units,
		const SymbolMap* imported_symbol_maps,
		const SymbolMap* exported_symbol_maps,
		const SymbolMap* dynamically_linked_symbols,
		size_t unit_count,
		String entry_point_name,
		Arena* allocator,
		Arena* temp_allocator,
		LinkedProgram* out_linked);

void linker_print_errors(const LinkerError* errors,
		size_t error_count,
		const SymbolMap* imported_symbol_maps,
		const SymbolMap* exported_symbol_maps);

#endif
