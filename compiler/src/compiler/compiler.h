#ifndef COMPILER_H
#define COMPILER_H

#include "parser/parsed_ast.h"
#include "code_gen/instr.h"

typedef struct {
	const ParsedVariable* var_def;
	InstrIndex value;
} VariableState;

typedef struct {
	const ParsedFunction* function;

	Arena* allocator;
	Arena* instr_allocator;
	Arena* temp_allocator;
	InstrBuffer instr_buffer;

	size_t var_count;
	VariableState* vars;
} FunctionCompiler;

void function_compiler_compile(FunctionCompiler* compiler);

#endif
