#include "compiler.h"

static InstrIndex _compile_expr(InstrBuffer* instr_buffer, Arena* instr_allocator, const ParsedExpr* expr) {
	switch (expr->kind) {
	case EXPR_CALL:
		break;
	case EXPR_BINARY:
		break;
	case EXPR_UNARY:
		break;
	case EXPR_FUNCTION_REFERENCE:
		break;
	case EXPR_VARIABLE_REFERENCE:
		break;
	case EXPR_INTEGER_LITERAL: {
		InstrIndex instr_index = instr_buffer_append(instr_buffer, instr_allocator);
		Instr* instr = instr_buffer_at(instr_buffer, instr_index);
		instr->kind = INSTR_CONST_64;
		instr->const_64.u64 = expr->int_literal.value;
		return instr_index;
	}
	case EXPR_STRING_LITERAL:
		break;
	}

	unreachable();
	return (InstrIndex) {};
}

void function_compiler_compile(FunctionCompiler* compiler) {
	const ParsedScope* body = compiler->function->body;
	for (const ParsedNode* node = body->nodes.first; node != NULL; node = node->next) {
		if (node->kind == AST_NODE_EXPR) {
			_compile_expr(&compiler->instr_buffer, compiler->instr_allocator, &node->expr);
		}
	}
}
