#ifndef COMPILER_H
#define COMPILER_H

#include "parser/ast.h"
#include "code_gen/instr.h"
#include "code_gen/code_gen.h"
#include "code_gen/abi.h"

typedef struct TypeLayout TypeLayout;
typedef struct TypeContext TypeContext;

struct TypeLayout {
	size_t size;
	size_t alignment;
};

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
	CONTROL_FLOW_BREAK,
	CONTROL_FLOW_CONTINUE,
} ControlFlowKind;

typedef struct ControlFlowStmt ControlFlowStmt;
struct ControlFlowStmt {
	ControlFlowKind kind;

	// A region where this `break` or `continue` statement appears
	InstrIndex region;
	
	// Var and arg values at the time of reaching the `break` or `continue` statement.
	InstrIndex* var_values;
	InstrIndex* arg_values;

	ControlFlowStmt* next;
};

typedef struct {
	const Function* function;

	Arena* allocator;

	Arena* instr_allocator;
	Arena* temp_allocator;
	InstrBuffer instr_buffer;

	InstrIndex io_state;

	// The total number of variables in the function.
	//
	// Invariant `var_count == function->var_count`.
	size_t var_count;
	// The size is `var_count`.
	// Index using the variable id.
	//
	// Each element is only assigned when the varialbe definition is encountered, otherwise it stays
	// as `NULL`.
	const Variable** vars;
	// The size is `var_count`.
	// Index using the variable id.
	//
	// Elements are assigned in the same way as for `vars`.
	const Scope** var_parent_scopes;
	// The size is `var_count`.
	// Index using the variable id.
	//
	// If variable definition has already been encountered, the corresponding element will contain a
	// valid instruction, otherwise `INVALID_INSTR_INDEX`
	InstrIndex* var_values;
	InstrIndex* arg_states;

	const TypeContext* type_context;

	StringStorage* str_storage;
	SymbolMap* symbol_map;

	AstNode* current_loop;
	ControlFlowStmt* current_control_flow_stmts;
	ControlFlowStmt* free_control_flow_stmt;

	// An array internal to the compiler, which is used to defer filling of the
	// `function_call_signatures`. The array is allocated using the `temp_allocator`.
	//
	// Capacity is `function_call_count`
	Call** function_calls;

	// Number of calls currently stored in `function_calls`.
	size_t function_call_count;

	// Signatures used to tell the backend how to call functions
	//
	// Size is `function->function_call_count`
	AbiSignature* function_call_signatures;
} FunctionCompiler;

typedef struct {
	InstrBuffer instr_buffer;
	InstrIndex start_region;

	StringArray string_consts;
	AbiSignature* function_call_signatures;
	size_t function_call_signature_count;
} CompiledFunction;

CompiledFunction function_compiler_compile(FunctionCompiler* compiler);
void compiler_resolve_default_func_refs(SymbolMap* map);

void compiler_create_function_import_symbol(const Function* function, Symbol* out_symbol);
void compiler_collect_imported_symbols(const AST* ast, SymbolMap* imported_symbols);

// Stores precomputed layouts for all the compound types in the AST
struct TypeContext {
	TypeLayout pointer_type_layout;
	TypeLayout* layouts;
	size_t** field_offsets;
};

void compute_compound_type_layouts(TypeContext* context, const AST* ast, Arena* allocator);

AbiSignature function_prototype_to_abi_signature(const TypeContext* type_context,
		const FunctionPrototype* proto,
		Allocator allocator);

#endif
