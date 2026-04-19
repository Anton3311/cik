#include "compiler.h"

static InstrIndex _compile_expr(FunctionCompiler* compiler, const ParsedExpr* expr) {
	InstrBuffer* instr_buffer = &compiler->instr_buffer;
	Arena* instr_allocator = compiler->instr_allocator;
	switch (expr->kind) {
	case EXPR_CALL:
		break;
	case EXPR_BINARY: {
		if (expr->binary.op == BIN_OP_ASSIGNMENT) {
			assert(expr->binary.left->kind == EXPR_VARIABLE_REFERENCE);

			InstrIndex value = _compile_expr(compiler, expr->binary.right);
			const ParsedVariable* variable = expr->binary.left->variable_ref;

			compiler->vars[variable->id].value = value;
			return value;
		}

		InstrIndex left = _compile_expr(compiler, expr->binary.left);
		InstrIndex right = _compile_expr(compiler, expr->binary.right);

		InstrIndex instr_index = instr_buffer_append(instr_buffer, instr_allocator);
		Instr* instr = instr_buffer_at(instr_buffer, instr_index);
		instr->kind = INSTR_BIN_OP_64;
		instr->bin_op.kind = INSTR_BIN_ADD;
		instr->bin_op.left = left;
		instr->bin_op.right = right;
		return instr_index;
	}
	case EXPR_UNARY:
		break;
	case EXPR_FUNCTION_REFERENCE:
		break;
	case EXPR_VARIABLE_REFERENCE: {
		InstrIndex var_value = compiler->vars[expr->variable_ref->id].value;
		assert(var_value.value != INVALID_INSTR_INDEX.value);
		return var_value;
	}
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
	if (!body) {
		return;
	}

	compiler->var_count = compiler->function->var_count;
	compiler->vars = arena_alloc_array(compiler->allocator, VariableState, compiler->var_count);

	instr_buffer_init(&compiler->instr_buffer, compiler->instr_allocator);

	for (size_t i = 0; i < compiler->var_count; i += 1) {
		compiler->vars[i].value = INVALID_INSTR_INDEX;
	}

	for (const ParsedNode* node = body->nodes.first; node != NULL; node = node->next) {
		switch (node->kind) {
		case AST_NODE_VARIABLE:
			assert(node->variable.id < compiler->var_count);

			compiler->vars[node->variable.id].var_def = &node->variable;
			if (node->variable.value) {
				compiler->vars[node->variable.id].value = _compile_expr(compiler, node->variable.value);
			}

			break;
		case AST_NODE_EXPR:
			_compile_expr(compiler, &node->expr);
			break;
		}
	}
}
