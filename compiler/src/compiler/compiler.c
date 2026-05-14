#include "compiler.h"

static InstrIndex _get_arg_load_instr(const FunctionCompiler* compiler, uint8_t arg_index) {
	const ParsedFunction* func = compiler->function;
	assert(arg_index < func->parameter_count);
	assert(arg_index < compiler->instr_buffer.count);
	return (InstrIndex) { .value = (uint16_t)arg_index };
}

static TypeLayout _type_get_layout(const FunctionCompiler* compiler, const ParsedType* type) {
	switch (type->kind) {
	case PARSED_TYPE_VOID:
		return type_layout_new(0, 0);

	case PARSED_TYPE_CHAR:
	case PARSED_TYPE_SIGNED_CHAR:
	case PARSED_TYPE_UNSIGNED_CHAR:
	case PARSED_TYPE_INT8:
	case PARSED_TYPE_SIGNED_INT8:
	case PARSED_TYPE_UNSIGNED_INT8:
		return type_layout_new(1, 1);
	case PARSED_TYPE_SHORT:
	case PARSED_TYPE_SIGNED_SHORT:
	case PARSED_TYPE_UNSIGNED_SHORT:
	case PARSED_TYPE_INT16:
	case PARSED_TYPE_SIGNED_INT16:
	case PARSED_TYPE_UNSIGNED_INT16:
		return type_layout_new(2, 2);
	case PARSED_TYPE_INT:
	case PARSED_TYPE_SIGNED_INT:
	case PARSED_TYPE_UNSIGNED_INT:
	case PARSED_TYPE_LONG:
	case PARSED_TYPE_SIGNED_LONG:
	case PARSED_TYPE_UNSIGNED_LONG:
	case PARSED_TYPE_INT32:
	case PARSED_TYPE_SIGNED_INT32:
	case PARSED_TYPE_UNSIGNED_INT32:
		return type_layout_new(4, 4);
	case PARSED_TYPE_LONG_LONG:
	case PARSED_TYPE_SIGNED_LONG_LONG:
	case PARSED_TYPE_UNSIGNED_LONG_LONG:
	case PARSED_TYPE_INT64:
	case PARSED_TYPE_SIGNED_INT64:
	case PARSED_TYPE_UNSIGNED_INT64:
		return type_layout_new(8, 8);

	case PARSED_TYPE_SIZE_T:
	case PARSED_TYPE_POINTER:
		return compiler->pointer_type_layout;

	case PARSED_TYPE_FLOAT:
		return type_layout_new(4, 4);
	case PARSED_TYPE_DOUBLE:
		return type_layout_new(8, 8);

	case PARSED_TYPE_STRUCT:
		break;
	case PARSED_TYPE_UNION:
		break;
	case PARSED_TYPE_ENUM:
		break;

	case PARSED_TYPE_ARRAY:
		break;
	}

	unreachable();
	return (TypeLayout) {};
}

static InstrIndex _compile_expr(FunctionCompiler* compiler, const ParsedExpr* expr) {
	InstrBuffer* instr_buffer = &compiler->instr_buffer;
	Arena* instr_allocator = compiler->instr_allocator;
	switch (expr->kind) {
	case EXPR_CALL: {
		const ParsedExpr* callable = expr->call.callable;
		assert(callable->kind == EXPR_FUNCTION_REFERENCE);

		assert(expr->call.args.count == 1);
		const ParsedExpr* first_arg = expr->call.args.exprs[0];

		InstrIndex call_instr_index = instr_buffer_append(instr_buffer, instr_allocator);
		Instr* call_instr = instr_buffer_at(instr_buffer, call_instr_index);
		call_instr->kind = INSTR_CALL_INTERNAL;
		call_instr->call_internal.arg = _compile_expr(compiler, first_arg);
		call_instr->call_internal.io_state = compiler->io_state;

		String func_name = callable->function_ref->name.string;
		if (str_equal(func_name, STR_LIT("assert"))) {
			call_instr->call_internal.function_index = 0;
		} else if (str_equal(func_name, STR_LIT("print_string"))) {
			call_instr->call_internal.function_index = 1;
		}

		compiler->io_state = instr_new_io_state(instr_buffer, instr_allocator, call_instr_index);
		return call_instr_index;
	}
	case EXPR_BINARY: {
		if (expr->binary.op == BIN_OP_ASSIGNMENT) {
			assert(expr->binary.left->kind == EXPR_VARIABLE_REFERENCE);

			InstrIndex value = _compile_expr(compiler, expr->binary.right);
			const ParsedVariable* variable = expr->binary.left->variable_ref;

			compiler->vars[variable->id].value = value;
			return value;
		}

		ParsedType* left_type = expr_get_type(expr->binary.left, compiler->temp_allocator);
		ParsedType* right_type = expr_get_type(expr->binary.right, compiler->temp_allocator);

		assert(left_type);
		assert(right_type);

		bool left_is_pointer_like = type_kind_is_pointer_like(left_type->kind);
		bool right_is_pointer_like = type_kind_is_pointer_like(right_type->kind);

		InstrIndex left = _compile_expr(compiler, expr->binary.left);
		InstrIndex right = _compile_expr(compiler, expr->binary.right);

		// TODO: Don't scale int constants during compare operations.

		// NOTE: In case we are doing pointer arithmetics here,
		//       and one of the operands is an interger, we need
		//       to scale that integer by the byte size of base pointer type.
		//
		//       Since during pointer arithmetics those integer constants
		//       encode an offset by a number of array elements and not bytes.
		if (left_is_pointer_like && type_kind_is_int(right_type->kind)) {
			ParsedType* base_type = type_extract_pointer_base_type(left_type);
			TypeLayout value_layout = _type_get_layout(compiler, base_type);

			assert(is_power_of_2(value_layout.size));
			size_t shift_count = count_trailing_zeros(value_layout.size);

			right = instr_new_logical_shift_left_by(instr_buffer,
					instr_allocator,
					right,
					(uint8_t)shift_count);
		} else if (right_is_pointer_like && type_kind_is_int(left_type->kind)) {
			ParsedType* base_type = type_extract_pointer_base_type(right_type);
			TypeLayout value_layout = _type_get_layout(compiler, base_type);

			assert(is_power_of_2(value_layout.size));
			size_t shift_count = count_trailing_zeros(value_layout.size);

			left = instr_new_logical_shift_left_by(instr_buffer,
					instr_allocator,
					left,
					(uint8_t)shift_count);
		}

		InstrIndex instr_index = instr_buffer_append(instr_buffer, instr_allocator);
		Instr* instr = instr_buffer_at(instr_buffer, instr_index);

		switch (expr->binary.op) {
		case BIN_OP_ADD:
			instr->kind = INSTR_BIN_OP_64;
			instr->bin_op.kind = INSTR_BIN_ADD;
			instr->bin_op.left = left;
			instr->bin_op.right = right;
			break;
		case BIN_OP_LOGICAL_EQUAL:
			instr->kind = INSTR_COMPARE_64;
			instr->compare.kind = INSTR_CMP_EQUAL;
			instr->compare.left = left;
			instr->compare.right = right;
			break;
		case BIN_OP_LOGICAL_NOT_EQUAL:
			instr->kind = INSTR_COMPARE_64;
			instr->compare.kind = INSTR_CMP_NOT_EQUAL;
			instr->compare.left = left;
			instr->compare.right = right;
			break;
		case BIN_OP_LOGICAL_LESS:
			instr->kind = INSTR_COMPARE_64;
			instr->compare.kind = INSTR_CMP_LESS;
			instr->compare.left = left;
			instr->compare.right = right;
			break;
		case BIN_OP_LOGICAL_GREATER:
			instr->kind = INSTR_COMPARE_64;
			instr->compare.kind = INSTR_CMP_GREATER;
			instr->compare.left = left;
			instr->compare.right = right;
			break;
		}

		if (bin_op_is_compare(expr->binary.op)) {
			return instr_new_cast(instr_buffer, instr_allocator, instr_index, 64);
		}

		return instr_index;
	}
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
		instr->const_64.u = expr->int_literal.value;
		return instr_index;
	}
	case EXPR_STRING_LITERAL:
		break;
	case EXPR_FUNCTION_PARAM: {
		assert(expr->function_param.param_index < UINT8_MAX);
		return _get_arg_load_instr(compiler, (uint8_t)expr->function_param.param_index);
	}
	case EXPR_UNARY:
		switch (expr->unary.op) {
		case UNARY_OP_DEREFERENCE: {
			InstrIndex operand_instr = _compile_expr(compiler, expr->unary.operand);

			ParsedType* operand_type = expr_get_type(expr->unary.operand, compiler->temp_allocator);

			const ParsedType* base_type = NULL;
			if (operand_type->kind == PARSED_TYPE_POINTER) {
				base_type = operand_type->pointer_base_type;
			} else if (operand_type->kind == PARSED_TYPE_ARRAY) {
				base_type = operand_type->array.element_type;
			} else {
				panic("todo: report error");
			}

			InstrIndex instr_index = instr_buffer_append(instr_buffer, instr_allocator);
			Instr* instr = instr_buffer_at(instr_buffer, instr_index);

			instr->ptr_load.ptr = operand_instr;

			TypeLayout layout = _type_get_layout(compiler, base_type);
			switch (layout.size) {
			case 1:
				instr->kind = INSTR_PTR_LOAD_8;
				break;
			case 2:
				instr->kind = INSTR_PTR_LOAD_16;
				break;
			case 4:
				instr->kind = INSTR_PTR_LOAD_32;
				break;
			case 8:
				instr->kind = INSTR_PTR_LOAD_64;
				break;
			default:
				panic("Only up to 8 byte sizes are supported for dereferencing");
			}

			return instr_index;
		}
		}
		break;
	}

	unreachable();
	return (InstrIndex) {};
}

static InstrIndex _compile_block_to_region(FunctionCompiler* compiler, ParsedNode* first_node) {
	InstrBuffer* instr_buffer = &compiler->instr_buffer;
	Arena* instr_allocator = compiler->instr_allocator;

	InstrIndex initial_region = instr_new_region(instr_buffer, instr_allocator);
	InstrIndex region_instr_index = initial_region;

	for (const ParsedNode* node = first_node; node != NULL; node = node->next) {
		Instr* region_instr = instr_buffer_at(instr_buffer, region_instr_index);

		switch (node->kind) {
		case AST_NODE_VARIABLE:
			assert(node->variable.id < compiler->var_count);

			compiler->vars[node->variable.id].var_def = &node->variable;
			if (node->variable.value) {
				compiler->vars[node->variable.id].value = _compile_expr(compiler, node->variable.value);
			}

			break;
		case AST_NODE_IF: {
			InstrIndex instr_index = instr_buffer_append(instr_buffer, instr_allocator);
			Instr* instr = instr_buffer_at(instr_buffer, instr_index);
			instr->kind = INSTR_BRANCH;
			instr->branch.condition = _compile_expr(compiler, &node->if_stmt.condition);

			InstrIndex post_branch_region_index = instr_new_region(instr_buffer, instr_allocator);
			InstrIndex post_branch_jump_index = instr_new_jump(instr_buffer, instr_allocator, post_branch_region_index);

			InstrIndex true_region_index = _compile_block_to_region(compiler, node->if_stmt.true_node);
			InstrIndex false_region_index = INVALID_INSTR_INDEX;

			{
				Instr* true_region = instr_buffer_at(instr_buffer, true_region_index);
				true_region->region.last_instr = post_branch_jump_index;
			}

			{
				false_region_index = instr_new_region(instr_buffer, instr_allocator);
				Instr* false_region = instr_buffer_at(instr_buffer, false_region_index);
				false_region->region.last_instr = post_branch_jump_index;
			}

			instr->branch.true_region = true_region_index;
			instr->branch.false_region = false_region_index;

			assert(node->if_stmt.false_node == NULL);

			region_instr->region.last_instr = instr_index;

			region_instr_index = post_branch_region_index;
			break;
		}
		case AST_NODE_BLOCK: {
			InstrIndex inner_region = _compile_block_to_region(compiler, node->block.nodes.first);
			InstrIndex jump_to_inner_region = instr_new_jump(instr_buffer, instr_allocator, inner_region);

			InstrIndex post_block_region = instr_new_region(instr_buffer, instr_allocator);
			InstrIndex jump_to_post_block_region = instr_new_jump(instr_buffer, instr_allocator, post_block_region);

			region_instr->region.last_instr = jump_to_inner_region;

			Instr* inner_region_instr = instr_buffer_at(instr_buffer, inner_region);
			inner_region_instr->region.last_instr = jump_to_post_block_region;

			region_instr_index = post_block_region;
			break;
		}
		case AST_NODE_RETURN:
			assert(node->return_stmt.value != NULL);
			InstrIndex value = _compile_expr(compiler, node->return_stmt.value);
			region_instr->region.last_instr = instr_new_return_value(instr_buffer, instr_allocator, value);
			break;
		case AST_NODE_EXPR:
			_compile_expr(compiler, &node->expr);
			break;
		}
	}

	return initial_region;
}

CompiledFunction function_compiler_compile(FunctionCompiler* compiler) {
	const ParsedScope* body = compiler->function->body;
	assert(body);

	compiler->var_count = compiler->function->var_count;
	compiler->vars = arena_alloc_array(compiler->allocator, VariableState, compiler->var_count);

	instr_buffer_init(&compiler->instr_buffer, compiler->instr_allocator);

	for (size_t i = 0; i < compiler->var_count; i += 1) {
		compiler->vars[i].value = INVALID_INSTR_INDEX;
	}

	InstrBuffer* instr_buffer = &compiler->instr_buffer;
	Arena* instr_allocator = compiler->instr_allocator;

	assert_msg(compiler->function->parameter_count <= 4, "For now only up to 4 params are supported");
	for (size_t i = 0; i < compiler->function->parameter_count; i += 1) {
		InstrIndex index = instr_buffer_append(instr_buffer, instr_allocator);
		Instr* instr = instr_buffer_at(instr_buffer, index);
		instr->kind = INSTR_LOAD_ARG;
		instr->load_arg.index = (uint8_t)i;
	}

	compiler->io_state = instr_new_io_state(instr_buffer, instr_allocator, INVALID_INSTR_INDEX);

	InstrIndex region = _compile_block_to_region(compiler, compiler->function->body->nodes.first);
	Instr* region_instr = instr_buffer_at(instr_buffer, region);
	region_instr->region.io_state = compiler->io_state;

	InstrUsageRange* usage_ranges = instr_compute_usage_ranges(compiler->instr_buffer,
			region,
			compiler->instr_allocator,
			compiler->temp_allocator);

	instr_print_all(compiler->instr_buffer);

	for (size_t i = 0; i < compiler->instr_buffer.count; i += 1) {
		printf("\t%zu: %u\t%u\n", i,
				(uint32_t)usage_ranges[i].first_usage.value,
				(uint32_t)usage_ranges[i].last_usage.value);
	}

	CompiledFunction compiled_function;
	compiled_function.instr_buffer = compiler->instr_buffer;
	compiled_function.usage_ranges = usage_ranges;
	compiled_function.start_region = region;
	return compiled_function;
}
