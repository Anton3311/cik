#ifndef COMPILER_H
#define COMPILER_H

#include "parser/parsed_ast.h"
#include "code_gen/instr.h"

typedef struct {
	const ParsedFunction* function;

	Arena* instr_allocator;
	Arena* temp_allocator;
	InstrBuffer instr_buffer;
} FunctionCompiler;

void function_compiler_compile(FunctionCompiler* compiler);

#endif
