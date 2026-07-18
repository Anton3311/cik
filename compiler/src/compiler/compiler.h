#ifndef COMPILER_H
#define COMPILER_H

#include "parser/ast.h"
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

typedef enum {
	LOOP_CONTROL_BREAK,
	LOOP_CONTROL_CONTINUE,
} LoopControlKind;

typedef struct LoopControlStmt LoopControlStmt;
struct LoopControlStmt {
	LoopControlKind kind;

	// A region where this `break` or `continue` statement appears
	InstrIndex region;
	
	// Var and arg values at the time of reaching the `break` or `continue` statement.
	InstrIndex* var_values;
	InstrIndex* arg_values;

	LoopControlStmt* next;
};

typedef struct {
	const Function* function;

	Arena* allocator;

	Arena* instr_allocator;
	Arena* temp_allocator;
	InstrBuffer instr_buffer;

	InstrIndex io_state;

	size_t var_count;
	const Variable** vars;
	const Scope** var_parent_scopes;
	InstrIndex* var_values;
	InstrIndex* arg_states;

	TypeLayout pointer_type_layout;

	StringStorage str_storage;
	FunctionRefTable func_ref_table;

	AstNode* current_loop;
	LoopControlStmt* current_loop_control_stmts;
	LoopControlStmt* free_loop_control_stmt;
} FunctionCompiler;

typedef struct {
	InstrBuffer instr_buffer;
	InstrIndex start_region;
	InstrLiveRange* live_ranges;

	FunctionRefTable func_ref_table;

	StringArray string_consts;
} CompiledFunction;

CompiledFunction function_compiler_compile(FunctionCompiler* compiler);
void compiler_resolve_default_func_refs(FunctionRefTable* table);

#endif
