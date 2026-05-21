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

		assert(expr->call.args.count <= UINT16_MAX);

		InstrInputs arg_inputs = instr_allocate_inputs_array(instr_buffer,
				expr->call.args.count,
				compiler->input_instr_array_allocator);

		for (uint16_t i = 0; i < arg_inputs.count; i += 1) {
			const ParsedExpr* arg= expr->call.args.exprs[i];
			instr_buffer->inputs_buffer[arg_inputs.start + i] = _compile_expr(compiler, arg);
		}

		InstrIndex call_instr_index = instr_buffer_append(instr_buffer, instr_allocator);
		Instr* call_instr = instr_buffer_at(instr_buffer, call_instr_index);
		call_instr->kind = INSTR_CALL_INTERNAL;
		call_instr->call_internal.args = arg_inputs;
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
			ParsedExpr* target = expr->binary.left;

			if (target->kind == EXPR_VARIABLE_REFERENCE) {
				InstrIndex value = _compile_expr(compiler, expr->binary.right);
				const ParsedVariable* variable = target->variable_ref;
				compiler->var_values[variable->id] = value;
				return value;
			} else if (target->kind == EXPR_FUNCTION_PARAM) {
				InstrIndex value = _compile_expr(compiler, expr->binary.right);
				size_t arg_index = target->function_param.param_index;
				compiler->arg_states[arg_index] = value;
				return value;
			} else {
				panic("Assignment to this expression kind is not supported");
			}

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
		InstrIndex var_value = compiler->var_values[expr->variable_ref->id];
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
		size_t arg_index = expr->function_param.param_index;
		assert(arg_index < compiler->function->parameter_count);
		return compiler->arg_states[arg_index];
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

static InstrIndex _create_phi_of_2_variants(FunctionCompiler* compiler,
		InstrIndex variant_a,
		InstrIndex region_a,
		InstrIndex variant_b,
		InstrIndex region_b) {
	InstrBuffer* instr_buffer = &compiler->instr_buffer;
	Arena* instr_allocator = compiler->instr_allocator;

	InstrIndex select_a_index = instr_buffer_append(instr_buffer, instr_allocator);
	Instr* select_a = instr_buffer_at(instr_buffer, select_a_index);
	select_a->kind = INSTR_SELECT;
	select_a->select.value = variant_a;
	select_a->select.region = region_a;

	InstrIndex select_b_index = instr_buffer_append(instr_buffer, instr_allocator);
	Instr* select_b = instr_buffer_at(instr_buffer, select_b_index);
	select_b->kind = INSTR_SELECT;
	select_b->select.value = variant_b;
	select_b->select.region = region_b;

	InstrInputs select_inputs_buffer = instr_allocate_inputs_array(instr_buffer,
			2, compiler->input_instr_array_allocator);

	InstrIndex* select_inputs = &instr_buffer->inputs_buffer[select_inputs_buffer.start];
	select_inputs[0] = select_a_index;
	select_inputs[1] = select_b_index;

	InstrIndex phi_index = instr_buffer_append(instr_buffer, instr_allocator);
	Instr* phi = instr_buffer_at(instr_buffer, phi_index);
	phi->kind = INSTR_PHI;
	phi->phi.variants = select_inputs_buffer;

	return phi_index;
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

			compiler->vars[node->variable.id] = &node->variable;
			compiler->var_parent_scopes[node->variable.id] = node->parent_scope;

			if (node->variable.value) {
				compiler->var_values[node->variable.id] = _compile_expr(compiler, node->variable.value);
			}

			break;
		case AST_NODE_IF: {
			// NOTE: How are branches compiled:
			//       
			//       Branches split the flow of the program into two possible paths,
			//       and at the end those two paths need to be merged back into one.
			//       
			//       For each alternative path we create a region, the branch instruction
			//       jumps to one of them, based on the condition.
			//       
			//       To merge these two paths we end both alternative paths with an
			//       unconditional jump to a third region. This third region is where
			//       the program flow continues further after the branch.
			InstrIndex instr_index = instr_buffer_append(instr_buffer, instr_allocator);
			Instr* instr = instr_buffer_at(instr_buffer, instr_index);
			instr->kind = INSTR_BRANCH;
			instr->branch.condition = _compile_expr(compiler, &node->if_stmt.condition);

			InstrIndex post_branch_region_index = instr_new_region(instr_buffer, instr_allocator);
			InstrIndex post_branch_jump_index = instr_new_jump(instr_buffer, instr_allocator, post_branch_region_index);

			InstrIndex true_region_index = INVALID_INSTR_INDEX;
			InstrIndex false_region_index = INVALID_INSTR_INDEX;

			{
				// NOTE: Here is the fun part: placing phi nodes
				//       We have an array where each variable stores it's currently
				//       assigned instruction (value).
				//       
				//       To decide where to place the phi nodes, we need to track
				//       which variables get assigned a new value during the compilation
				//       of each of the branch's alternative paths.
				//       
				//       This is done by creating copies of the original array of variable
				//       values for each branch path, then compiling the paths with the
				//       replaced arrays for variable values. In that way we gather newly
				//       assigned value and later are able to make a placement decision.

				ArenaRegion temp = arena_begin_temp(compiler->temp_allocator);

				InstrIndex* original_var_values = compiler->var_values;

				// Create copies of variable value arrays
				InstrIndex* var_values_for_true_path = arena_alloc_array(compiler->temp_allocator,
						InstrIndex,
						compiler->var_count);
				InstrIndex* var_values_for_false_path = arena_alloc_array(compiler->temp_allocator,
						InstrIndex,
						compiler->var_count);

				array_copy(var_values_for_true_path, compiler->var_values, compiler->var_count);
				array_copy(var_values_for_false_path, compiler->var_values, compiler->var_count);

				{
					compiler->var_values = var_values_for_true_path;

					true_region_index = _compile_block_to_region(compiler, node->if_stmt.true_node);
					Instr* true_region = instr_buffer_at(instr_buffer, true_region_index);
					true_region->region.last_instr = post_branch_jump_index;
				}

				{
					compiler->var_values = var_values_for_false_path;

					if (node->if_stmt.false_node) {
						false_region_index = _compile_block_to_region(compiler, node->if_stmt.false_node);
					} else {
						false_region_index = instr_new_region(instr_buffer, instr_allocator);
					}

					Instr* false_region = instr_buffer_at(instr_buffer, false_region_index);
					false_region->region.last_instr = post_branch_jump_index;
				}

				const ParsedScope* if_parent_scope = node->parent_scope;
				for (size_t i = 0; i < compiler->var_count; i += 1) {
					if (compiler->vars[i] == NULL) {
						continue;
					}

					const ParsedScope* var_parent_scope = compiler->var_parent_scopes[i];
					if (var_parent_scope->id > if_parent_scope->id) {
						// The variable is defined deeper down the scopes hierarachy,
						// so it must have been defined in one of the if statement
						// branches. Which means phi placement doesn't apply here.
						continue;
					}

					bool assigned_in_true_path =
						var_values_for_true_path[i].value != original_var_values[i].value;
					bool assigned_in_false_path =
						var_values_for_false_path[i].value != original_var_values[i].value;

					if (!assigned_in_true_path && !assigned_in_false_path) {
						// No need to place a phi node, since no new values were assigned
						continue;
					}

					InstrIndex phi = _create_phi_of_2_variants(compiler,
							var_values_for_true_path[i],
							true_region_index,
							var_values_for_false_path[i],
							false_region_index);

					original_var_values[i] = phi;
				}

				arena_end_temp(temp);

				// Reset back to the original array of values
				compiler->var_values = original_var_values;
			}

			instr->branch.true_region = true_region_index;
			instr->branch.false_region = false_region_index;

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
		case AST_NODE_RETURN: {
			assert(node->return_stmt.value != NULL);
			InstrIndex value = _compile_expr(compiler, node->return_stmt.value);
			region_instr->region.last_instr = instr_new_return_value(instr_buffer, instr_allocator, value);
			break;
		}
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

	// Allocate var states buffer
	compiler->var_count = compiler->function->var_count;
	compiler->vars = arena_alloc_array_zeroed(compiler->allocator, const ParsedVariable*, compiler->var_count);
	compiler->var_values = arena_alloc_array(compiler->allocator, InstrIndex, compiler->var_count);
	compiler->var_parent_scopes = arena_alloc_array_zeroed(compiler->allocator, const ParsedScope*, compiler->var_count);

	for (size_t i = 0; i < compiler->var_count; i += 1) {
		compiler->var_values[i] = INVALID_INSTR_INDEX;
	}

	// Allocate`arg_states` buffer
	assert_msg(compiler->function->parameter_count <= 4, "For now only up to 4 params are supported");
	compiler->arg_states = arena_alloc_array(compiler->allocator,
			InstrIndex,
			compiler->function->parameter_count);

	// Init `InstrBuffer`
	InstrBuffer* instr_buffer = &compiler->instr_buffer;
	Arena* instr_allocator = compiler->instr_allocator;

	instr_buffer->inputs_buffer = arena_alloc_array(compiler->input_instr_array_allocator, InstrIndex, 0);
	instr_buffer->inputs_buffer_size = 0;
	instr_buffer_init(instr_buffer, instr_allocator);

	// Setup initial `INSTR_LOAD_ARG`
	for (size_t i = 0; i < compiler->function->parameter_count; i += 1) {
		InstrIndex index = instr_buffer_append(instr_buffer, instr_allocator);
		Instr* instr = instr_buffer_at(instr_buffer, index);
		instr->kind = INSTR_LOAD_ARG;
		instr->load_arg.index = (uint8_t)i;

		compiler->arg_states[i] = index;
	}

	compiler->io_state = instr_new_io_state(instr_buffer, instr_allocator, INVALID_INSTR_INDEX);

	InstrIndex region = _compile_block_to_region(compiler, compiler->function->body->nodes.first);
	Instr* region_instr = instr_buffer_at(instr_buffer, region);
	region_instr->region.io_state = compiler->io_state;

	InstrUsageRange* usage_ranges = instr_compute_usage_ranges(compiler->instr_buffer,
			region,
			compiler->instr_allocator,
			compiler->temp_allocator);

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
