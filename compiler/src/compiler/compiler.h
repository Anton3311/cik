#ifndef COMPILER_H
#define COMPILER_H

#include "parser/parsed_ast.h"
#include "code_gen/instr.h"

typedef struct {
	const ParsedVariable* var_def;
	InstrIndex value;
} VariableState;

typedef struct {
	size_t size;
	size_t alignment;
} TypeLayout;

inline TypeLayout type_layout_new(size_t size, size_t alignment) {
	return (TypeLayout) { .size = size, .alignment = alignment };
}

typedef struct {
	const ParsedFunction* function;

	Arena* allocator;

	// Some instructions have a variable number of inputs.
	// This allocator is exclusively used for allocating these arrays of `InstrIndex`
	Arena* input_instr_array_allocator;
	Arena* instr_allocator;
	Arena* temp_allocator;
	InstrBuffer instr_buffer;

	InstrIndex io_state;

	size_t var_count;
	VariableState* vars;

	TypeLayout pointer_type_layout;
} FunctionCompiler;

typedef struct {
	InstrBuffer instr_buffer;
	InstrIndex start_region;
	InstrUsageRange* usage_ranges;
} CompiledFunction;

CompiledFunction function_compiler_compile(FunctionCompiler* compiler);

#endif
