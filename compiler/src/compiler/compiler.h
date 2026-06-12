#ifndef COMPILER_H
#define COMPILER_H

#include "parser/parsed_ast.h"
#include "code_gen/instr.h"
#include "code_gen/code_gen.h"

typedef struct {
	size_t size;
	size_t alignment;
} TypeLayout;

inline TypeLayout type_layout_new(size_t size, size_t alignment) {
	return (TypeLayout) { .size = size, .alignment = alignment };
}

//
// StringStorage
//

typedef struct {
	Allocator allocator;

	String* strings;
	uint32_t count;
	uint32_t capacity;
} StringStorage;

uint32_t str_storage_append(StringStorage* storage, String string);
void str_storage_release(StringStorage* storage);

inline StringArray str_storage_to_array(StringStorage* storage) {
	return (StringArray) { .values = storage->strings, .count = storage->count };
}

//
// FunctionCompiler
//

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
	const ParsedVariable** vars;
	const ParsedScope** var_parent_scopes;
	InstrIndex* var_values;
	InstrIndex* arg_states;

	TypeLayout pointer_type_layout;

	StringStorage str_storage;
	FunctionRefTable func_ref_table;
} FunctionCompiler;

typedef struct {
	InstrBuffer instr_buffer;
	InstrIndex start_region;
	InstrUsageRange* usage_ranges;

	FunctionRefTable func_ref_table;

	StringArray string_consts;
} CompiledFunction;

CompiledFunction function_compiler_compile(FunctionCompiler* compiler);
void compiler_resolve_default_func_refs(FunctionRefTable* table);

#endif
