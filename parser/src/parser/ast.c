#include "ast.h"

bool type_is_struct(const Type* type, const Struct* struct_def) {
	assert(struct_def->layout_kind == STRUCT_LAYOUT_KIND_STRUCT);

	if (type->kind != PARSED_TYPE_STRUCT) {
		return false;
	}

	return type->struct_def == struct_def;
}

bool type_is_enum(const Type* type, const Enum* enum_def) {
	if (type->kind != PARSED_TYPE_ENUM) {
		return false;
	}

	return type->enum_def == enum_def;
}

bool type_equal(const Type* a, const Type* b) {
	TypeKind a_without_signed = a->kind & (~TYPE_FLAG_SIGNED);
	TypeKind b_without_signed = b->kind & (~TYPE_FLAG_SIGNED);

	if (a_without_signed != b_without_signed) {
		return false;
	}

	if (a->qualifiers != b->qualifiers) {
		return false;
	}

	switch (a->kind) {
	case PARSED_TYPE_STRUCT:
		return a->struct_def == b->struct_def;
	case PARSED_TYPE_UNION:
		return a->union_def == b->union_def;
	case PARSED_TYPE_ENUM:
		return a->enum_def == b->enum_def;
	case PARSED_TYPE_VOID:
	case PARSED_TYPE_SIZE_T:

	case PARSED_TYPE_CHAR:
	case PARSED_TYPE_INT:
	case PARSED_TYPE_SHORT:
	case PARSED_TYPE_LONG:
	case PARSED_TYPE_LONG_LONG:
	case PARSED_TYPE_INT8:
	case PARSED_TYPE_INT16:
	case PARSED_TYPE_INT32:
	case PARSED_TYPE_INT64:

	case PARSED_TYPE_SIGNED_CHAR:
	case PARSED_TYPE_SIGNED_INT:
	case PARSED_TYPE_SIGNED_SHORT:
	case PARSED_TYPE_SIGNED_LONG:
	case PARSED_TYPE_SIGNED_LONG_LONG:
	case PARSED_TYPE_SIGNED_INT8:
	case PARSED_TYPE_SIGNED_INT16:
	case PARSED_TYPE_SIGNED_INT32:
	case PARSED_TYPE_SIGNED_INT64:


	case PARSED_TYPE_UNSIGNED_CHAR:
	case PARSED_TYPE_UNSIGNED_INT:
	case PARSED_TYPE_UNSIGNED_SHORT:
	case PARSED_TYPE_UNSIGNED_LONG:
	case PARSED_TYPE_UNSIGNED_LONG_LONG:
	case PARSED_TYPE_UNSIGNED_INT8:
	case PARSED_TYPE_UNSIGNED_INT16:
	case PARSED_TYPE_UNSIGNED_INT32:
	case PARSED_TYPE_UNSIGNED_INT64:

	case PARSED_TYPE_FLOAT:
	case PARSED_TYPE_DOUBLE:
		return true;
	
	case PARSED_TYPE_POINTER:
		assert(a->pointer_base_type != NULL);
		assert(b->pointer_base_type != NULL);
		return type_equal(a->pointer_base_type, b->pointer_base_type);
	
	case PARSED_TYPE_ARRAY:
		assert(a->array.size == NULL);
		assert(b->array.size == NULL);
		return type_equal(a->array.element_type, a->array.element_type);
	}

	unreachable();
	return false;
}

void type_array_to_pointer(const Type* type, Type* out_type) {
	assert(type->kind == PARSED_TYPE_ARRAY);

	Type* element_type = type->array.element_type;

	out_type->kind = PARSED_TYPE_POINTER;
	out_type->pointer_base_type = element_type;
}

uint32_t type_get_int_convertion_rank(const Type* type) {
	switch (type->kind) {
	case PARSED_TYPE_VOID:
		unreachable();

	case PARSED_TYPE_CHAR:
	case PARSED_TYPE_SIGNED_CHAR:
	case PARSED_TYPE_UNSIGNED_CHAR:
	case PARSED_TYPE_INT8:
	case PARSED_TYPE_SIGNED_INT8:
	case PARSED_TYPE_UNSIGNED_INT8:
		return 1;

	case PARSED_TYPE_SHORT:
	case PARSED_TYPE_SIGNED_SHORT:
	case PARSED_TYPE_UNSIGNED_SHORT:
	case PARSED_TYPE_INT16:
	case PARSED_TYPE_SIGNED_INT16:
	case PARSED_TYPE_UNSIGNED_INT16:
		return 2;

	case PARSED_TYPE_INT:
	case PARSED_TYPE_SIGNED_INT:
	case PARSED_TYPE_UNSIGNED_INT:
	case PARSED_TYPE_LONG:
	case PARSED_TYPE_SIGNED_LONG:
	case PARSED_TYPE_UNSIGNED_LONG:
	case PARSED_TYPE_INT32:
	case PARSED_TYPE_SIGNED_INT32:
	case PARSED_TYPE_UNSIGNED_INT32:
		return 3;

	case PARSED_TYPE_LONG_LONG:
	case PARSED_TYPE_SIGNED_LONG_LONG:
	case PARSED_TYPE_UNSIGNED_LONG_LONG:
	case PARSED_TYPE_INT64:
	case PARSED_TYPE_SIGNED_INT64:
	case PARSED_TYPE_UNSIGNED_INT64:
		return 4;
	
	case PARSED_TYPE_SIZE_T:
		return 5;

	case PARSED_TYPE_FLOAT:
	case PARSED_TYPE_DOUBLE:
		break;

	case PARSED_TYPE_STRUCT:
	case PARSED_TYPE_UNION:
	case PARSED_TYPE_ENUM:
		break;

	case PARSED_TYPE_POINTER:
	case PARSED_TYPE_ARRAY:
		break;
	}

	unreachable_msg("The provided type is not an integer and this doesn't have a convertion rank");
	return 0;
}

static String s_bin_op_kind_to_string[] = {
	[BIN_OP_ADD] = STR_LIT("+"),
	[BIN_OP_SUB] = STR_LIT("-"),
	[BIN_OP_MUL] = STR_LIT("*"),
	[BIN_OP_DIV] = STR_LIT("/"),
	[BIN_OP_MOD] = STR_LIT("%"),

	[BIN_OP_LOGICAL_AND] = STR_LIT("&&"),
	[BIN_OP_LOGICAL_OR] = STR_LIT("||"),

	[BIN_OP_LOGICAL_EQUAL] = STR_LIT("=="),
	[BIN_OP_LOGICAL_NOT_EQUAL] = STR_LIT("!="),
	[BIN_OP_LOGICAL_LESS] = STR_LIT("<"),
	[BIN_OP_LOGICAL_GREATER] = STR_LIT(">"),
	[BIN_OP_LOGICAL_LESS_OR_EQUAL] = STR_LIT("<="),
	[BIN_OP_LOGICAL_GREATER_OR_EQUAL] = STR_LIT(">="),

	[BIN_OP_BITWISE_AND] = STR_LIT("&"),
	[BIN_OP_BITWISE_OR] = STR_LIT("|"),
	[BIN_OP_BITWISE_XOR] = STR_LIT("^"),
	[BIN_OP_BITWISE_SHIFT_LEFT] = STR_LIT("<<"),
	[BIN_OP_BITWISE_SHIFT_RIGHT] = STR_LIT(">>"),

	[BIN_OP_ASSIGNMENT] = STR_LIT("="),

	[BIN_OP_ASSIGNMENT_BY_SUM] = STR_LIT("+="),
	[BIN_OP_ASSIGNMENT_BY_DIFFERENCE] = STR_LIT("-="),
	[BIN_OP_ASSIGNMENT_BY_PRODUCT] = STR_LIT("*="),
	[BIN_OP_ASSIGNMENT_BY_QUOTIENT] = STR_LIT("/="),
	[BIN_OP_ASSIGNMENT_BY_REMAINDER] = STR_LIT("%="),

	[BIN_OP_ASSIGNMENT_BY_BITWISE_AND] = STR_LIT("&="),
	[BIN_OP_ASSIGNMENT_BY_BITWISE_OR] = STR_LIT("|="),
	[BIN_OP_ASSIGNMENT_BY_BITWISE_XOR] = STR_LIT("^="),
	[BIN_OP_ASSIGNMENT_BY_BITWISE_SHIFT_LEFT] = STR_LIT("<<="),
	[BIN_OP_ASSIGNMENT_BY_BITWISE_SHIFT_RIGHT] = STR_LIT(">>="),
};

String bin_op_kind_to_string(BinOpKind op) {
	return s_bin_op_kind_to_string[op];
}

String unary_op_kind_to_string(UnaryOpKind op) {
	switch (op) {
	case UNARY_OP_NEGATE:
		return STR_LIT("-");
	case UNARY_OP_PLUS:
		return STR_LIT("+");
	case UNARY_OP_ADDRESS:
		return STR_LIT("&");
	case UNARY_OP_DEREFERENCE:
		return STR_LIT("*");
	case UNARY_OP_LOGICAL_NOT:
		return STR_LIT("!");
	case UNARY_OP_BITWISE_NOT:
		return STR_LIT("~");
	case UNARY_OP_PRE_INCREMENT:
		return STR_LIT("pre ++");
	case UNARY_OP_POST_INCREMENT:
		return STR_LIT("post ++");
	case UNARY_OP_PRE_DECREMENT:
		return STR_LIT("pre --");
	case UNARY_OP_POST_DECREMENT:
		return STR_LIT("post --");
	}

	unreachable();
	return (String) {};
}

uint32_t bin_op_precedence(BinOpKind op) {
	switch (op) {
	case BIN_OP_ADD:
	case BIN_OP_SUB:
		return 4;

	case BIN_OP_MUL:
	case BIN_OP_DIV:
	case BIN_OP_MOD:
		return 3;

	case BIN_OP_LOGICAL_AND:
		return 11;
	case BIN_OP_LOGICAL_OR:
		return 12;

	case BIN_OP_LOGICAL_EQUAL:
	case BIN_OP_LOGICAL_NOT_EQUAL:
		return 7;

	case BIN_OP_LOGICAL_LESS:
	case BIN_OP_LOGICAL_GREATER:
	case BIN_OP_LOGICAL_LESS_OR_EQUAL:
	case BIN_OP_LOGICAL_GREATER_OR_EQUAL:
		return 6;

	case BIN_OP_BITWISE_SHIFT_LEFT:
	case BIN_OP_BITWISE_SHIFT_RIGHT:
		return 5;

	case BIN_OP_BITWISE_AND:
		return 8;
	case BIN_OP_BITWISE_OR:
		return 10;
	case BIN_OP_BITWISE_XOR:
		return 9;

	case BIN_OP_ASSIGNMENT:
	case BIN_OP_ASSIGNMENT_BY_SUM:
	case BIN_OP_ASSIGNMENT_BY_DIFFERENCE:
	case BIN_OP_ASSIGNMENT_BY_PRODUCT:
	case BIN_OP_ASSIGNMENT_BY_QUOTIENT:
	case BIN_OP_ASSIGNMENT_BY_REMAINDER:

	case BIN_OP_ASSIGNMENT_BY_BITWISE_AND:
	case BIN_OP_ASSIGNMENT_BY_BITWISE_OR:
	case BIN_OP_ASSIGNMENT_BY_BITWISE_XOR:
	case BIN_OP_ASSIGNMENT_BY_BITWISE_SHIFT_LEFT:
	case BIN_OP_ASSIGNMENT_BY_BITWISE_SHIFT_RIGHT:
		return 14;
	}

	unreachable();
	return UINT32_MAX;
}

void bin_expr_select_result_type(const Type* left_type,
		const Type* right_type,
		Type* out_type) {

	if (left_type->kind == PARSED_TYPE_POINTER && type_kind_is_int(right_type->kind)) {
		*out_type = *left_type;
		return;
	}

	if (left_type->kind == PARSED_TYPE_ARRAY && type_kind_is_int(right_type->kind)) {
		type_array_to_pointer(left_type, out_type);
		return;
	}

	if (right_type->kind == PARSED_TYPE_POINTER && type_kind_is_int(left_type->kind)) {
		*out_type = *right_type;
		return;
	}

	if (right_type->kind == PARSED_TYPE_ARRAY && type_kind_is_int(left_type->kind)) {
		type_array_to_pointer(right_type, out_type);
		return;
	}

	if (left_type->kind == PARSED_TYPE_POINTER && right_type->kind == PARSED_TYPE_POINTER) {
		assert(type_equal(left_type, right_type));
		*out_type = *left_type;
		return;
	}

	assert(type_kind_is_int(left_type->kind));
	assert(type_kind_is_int(right_type->kind));

	uint32_t left_convertion_rank = type_get_int_convertion_rank(left_type);
	uint32_t right_convertion_rank = type_get_int_convertion_rank(right_type);
	if (left_convertion_rank == right_convertion_rank) {
		if (left_type->kind == PARSED_TYPE_SIZE_T || right_type->kind == PARSED_TYPE_SIZE_T) {
			out_type->kind = PARSED_TYPE_SIZE_T;
		} else if (has_flag(left_type->kind, TYPE_FLAG_UNSIGNED)) {
			*out_type = *left_type;
		} else if (has_flag(right_type->kind, TYPE_FLAG_UNSIGNED)) {
			*out_type = *right_type;
		} else {
			*out_type = *left_type;
		}
	} else if (left_convertion_rank > right_convertion_rank) {
		*out_type = *left_type;
	} else  {
		*out_type = *right_type;
	}
}

String function_calling_convetion_to_string(FunctionCallingConvention conv) {
	switch (conv) {
	case FUNC_CALL_CONV_DEFAULT:
		return STR_LIT("default");
	case FUNC_CALL_CONV_CDECL:
		return STR_LIT("__cdecl");
	}

	unreachable();
	return (String) {};
}

//
// AST
//

void parsed_node_list_append(NodeList* list, AstNode* node) {
	assert(list != NULL);
	assert(node != NULL);
	assert(node->next == NULL);

	if (list->first == NULL) {
		assert(list->last == NULL);
		assert(list->count == 0);

		list->first = node;
		list->last = node;
		list->count = 1;
	} else {
		assert(list->last != NULL);
		list->last->next = node;
		list->last = node;
		list->count += 1;
	}
}

static Type s_char_type = (Type) { .kind = PARSED_TYPE_CHAR };

void expr_get_type(Expr* expr, Type* out_type) {
	switch (expr->kind) {
	case EXPR_CALL: {
		Expr* callable = expr->call.callable;
		assert_msg(callable->kind == EXPR_FUNCTION_REFERENCE, "A callable expression is not function");

		*out_type = callable->function_ref->return_type;
		return;
	}
	case EXPR_BINARY: {
		out_type->kind = expr->binary.result_type_kind;
		out_type->pointer_base_type = expr->binary.pointer_base_type;
		return;
	}
	case EXPR_UNARY: {
		switch (expr->unary.op) {
		case UNARY_OP_NEGATE:
		case UNARY_OP_PLUS:
		case UNARY_OP_LOGICAL_NOT:

		case UNARY_OP_BITWISE_NOT:

		case UNARY_OP_PRE_INCREMENT:
		case UNARY_OP_POST_INCREMENT:

		case UNARY_OP_PRE_DECREMENT:
		case UNARY_OP_POST_DECREMENT:
			expr_get_type(expr->unary.operand, out_type);
			break;
		case UNARY_OP_DEREFERENCE: {
			Type operand_type;
			expr_get_type(expr->unary.operand, &operand_type);

			assert_msg(type_kind_is_pointer_like(operand_type.kind),
					"Dereferencing a non-pointer like type is not allowed");

			const Type* base_type = type_extract_pointer_base_type(&operand_type);
			*out_type = *base_type;
			break;
		}
		}

		return;
	}
	case EXPR_FUNCTION_REFERENCE:
		panic("todo: return a type that correspond to the function signature (function pointer type)");
	case EXPR_VARIABLE_REFERENCE:
		*out_type = expr->variable_ref->type;
		return;
	case EXPR_INTEGER_LITERAL: {
		out_type->kind = expr->int_literal.integer_type;
		return;
	}
	case EXPR_STRING_LITERAL:
		out_type->kind = PARSED_TYPE_POINTER;
		out_type->qualifiers = TYPE_QUALIFIER_CONST;
		out_type->pointer_base_type = &s_char_type;
		return;
	case EXPR_CHAR_LITERAL:
		out_type->kind = PARSED_TYPE_CHAR;
		return;
	case EXPR_ENUM_CONSTANT:
		break;
	case EXPR_FUNCTION_PARAM: {
		const Function* func = expr->function_param.function_def;
		assert(expr->function_param.param_index < func->parameter_count);
		*out_type = func->parameters[expr->function_param.param_index].type;
		return;
	}
	case EXPR_ARRAY_INDEX: {
		Type array_type;
		expr_get_type(expr->array_index.array, &array_type);

		Type* element_type = type_extract_pointer_base_type(&array_type);
		*out_type = *element_type;
		return;
	}
	}

	unreachable_msg("Failed to get expr type");
	return;
}

size_t struct_field_namespace_index_of(const StructFieldNamespace* struct_namespace, String name) {
	size_t index = hash_string(name) % struct_namespace->capacity;
	
	while (true) {
		String key = struct_namespace->keys[index];
		if (key.v == NULL) {
			return SIZE_MAX;
		} else if (str_equal(key, name)) {
			return index;
		}

		index = (index + 1) % struct_namespace->capacity;
	}

	unreachable();
	return SIZE_MAX;
}

typedef struct {
	size_t indent;
} PrinterState;

void printer_indent(const PrinterState* printer) {
for (size_t i = 0; i < printer->indent; i += 1) {
		printf("  ");
	}
}

void printer_begin_struct(PrinterState* printer, const char* struct_name) {
	printf("%s {\n", struct_name);
	printer->indent += 1;
}

void printer_end_struct(PrinterState* printer) {
	assert(printer->indent > 0);
	printer->indent -= 1;
	printer_indent(printer);
	printf("}\n");
}

void printer_begin_array(PrinterState* printer) {
	printf("[\n");
	printer->indent += 1;
}

void printer_end_array(PrinterState* printer) {
	assert(printer->indent > 0);
	printer->indent -= 1;
	printer_indent(printer);
	printf("]\n");
}

void printer_array_element(PrinterState* printer, size_t index) {
	printer_indent(printer);
	printf("%zu = ", index);
}

void printer_field(PrinterState* printer, const char* field_name) {
	printer_indent(printer);
	printf("%s = ", field_name);
}

void printer_string_value(PrinterState* printer, String value) {
	printf("%.*s\n", STR_FMT(value));
}

void printer_string_field(PrinterState* printer, const char* name, String value) {
	printer_indent(printer);
	printf("%s = %.*s\n", name, STR_FMT(value));
}

void printer_bool_field(PrinterState* printer, const char* name, bool value) {
	printer_indent(printer);
	printf("%s = %s\n", name, (value ? "true" : "false"));
}

//
// AST Printing
//

void print_type(PrinterState* printer, const Type* type);
void print_single_node(PrinterState* printer, const AstNode* node);
void print_decl_spec(PrinterState* printer, const DeclSpec* decl_spec) {
	String decl_spec_name = {};

	switch (decl_spec->kind) {
	case DECL_SPEC_DEPRECATED:
		decl_spec_name = STR_LIT("deprecated");
		break;
	case DECL_SPEC_NO_INLINE:
		decl_spec_name = STR_LIT("noinline");
		break;
	case DECL_SPEC_NO_RETURN:
		decl_spec_name = STR_LIT("noreturn");
		break;
	case DECL_SPEC_DLL_IMPORT:
		decl_spec_name = STR_LIT("dllimport");
		break;
	case DECL_SPEC_DLL_EXPORT:
		decl_spec_name = STR_LIT("dllexport");
		break;
	case DECL_SPEC_RESTRICT:
		decl_spec_name = STR_LIT("restrict");
		break;
	}

	printer_begin_struct(printer, "decl_spec");
	printer_string_field(printer, "name", decl_spec_name);

	if (decl_spec->kind == DECL_SPEC_DEPRECATED) {
		printer_string_field(printer, "deprecation_text", decl_spec->deprecation_text.full_string);
	}

	printer_end_struct(printer);
}

void print_expr(PrinterState* printer, const Expr* expr) {
	assert(expr != NULL);

	switch (expr->kind) {
	case EXPR_FUNCTION_REFERENCE:
		printer_begin_struct(printer, "function_ref");
		printer_string_field(printer, "name", expr->function_ref->name.string);
		printer_end_struct(printer);
		break;
	case EXPR_VARIABLE_REFERENCE:
		printer_begin_struct(printer, "variable_ref");
		printer_string_field(printer, "name", expr->variable_ref->name.string);
		printer_end_struct(printer);
		break;
	case EXPR_BINARY: {
		Type result_type = {
			.kind = expr->binary.result_type_kind,
			.pointer_base_type = expr->binary.pointer_base_type
		};

		printer_begin_struct(printer, "binary_expr");
		printer_string_field(printer, "kind", bin_op_kind_to_string(expr->binary.op));
		printer_field(printer, "result_type");
		print_type(printer, &result_type);
		printer_field(printer, "left");
		print_expr(printer, expr->binary.left);
		printer_field(printer, "right");
		print_expr(printer, expr->binary.right);
		printer_end_struct(printer);
		break;
	}
	case EXPR_UNARY:
		printer_begin_struct(printer, "unary_expr");
		printer_string_field(printer, "kind", unary_op_kind_to_string(expr->unary.op));
		printer_field(printer, "operand");
		print_expr(printer, expr->unary.operand);
		printer_end_struct(printer);
		break;
	case EXPR_INTEGER_LITERAL:
		printer_begin_struct(printer, "int_literal");
		printer_string_field(printer, "format", int_literal_format_to_string(expr->int_literal.format));
		printer_field(printer, "type");

		Type type = { .kind = expr->int_literal.integer_type };
		print_type(printer, &type);
		printer_field(printer, "value");
		printf("%llu\n", expr->int_literal.value);
		printer_end_struct(printer);
		break;
	case EXPR_STRING_LITERAL:
		printer_begin_struct(printer, "string");
		printer_string_field(printer, "value", expr->string_literal.full_string);
		printer_end_struct(printer);
		break;
	case EXPR_CHAR_LITERAL:
		printer_begin_struct(printer, "char");
		printer_field(printer, "value");
		printf("%u %c\n", expr->char_literal.value, (char)(expr->char_literal.value));
		printer_end_struct(printer);
		break;
	case EXPR_CALL: {
		printer_begin_struct(printer, "call");
		printer_field(printer, "callable");
		print_expr(printer, expr->call.callable);

		printer_field(printer, "args");
		printer_begin_array(printer);
		printer_end_array(printer);

		for (size_t i = 0; i < expr->call.args.count; i += 1) {
			printer_array_element(printer, i);
			print_expr(printer, expr->call.args.exprs[i]);
		}

		printer_end_struct(printer);
		break;
	}
	case EXPR_ENUM_CONSTANT: {
		const Enum* enum_def = expr->enum_constant.enum_def;
		printer_begin_struct(printer, "enum_constant");
		printer_string_field(printer, "enum_name", enum_def->name.string);
		printer_string_field(printer, "variant_name", enum_def->variants[expr->enum_constant.variant_index].name.string);
		printer_end_struct(printer);
		break;
	}
	case EXPR_FUNCTION_PARAM: {
		const Function* func_def = expr->function_param.function_def;
		printer_begin_struct(printer, "function_param");
		printer_string_field(printer, "func_name", func_def->name.string);
		printer_string_field(printer, "param_name", func_def->parameters[expr->function_param.param_index].name.string);
		printer_end_struct(printer);
		break;
	}
	case EXPR_ARRAY_INDEX: {
		printer_begin_struct(printer, "array_index");
		printer_field(printer, "array");
		print_expr(printer, expr->array_index.array);
		printer_field(printer, "index");
		print_expr(printer, expr->array_index.index);
		printer_end_struct(printer);
		break;
	}
	}
}

void print_struct_def(PrinterState* printer, const Struct* struct_def) {
	assert(struct_def != NULL);

	printer_begin_struct(printer, struct_def->layout_kind == STRUCT_LAYOUT_KIND_STRUCT
			? "struct"
			: "union");

	printer_string_field(printer, "name", struct_def->name.string);
	printer_bool_field(printer, "is_forward_declared", struct_def->is_forward_declared);
	
	if (!struct_def->is_forward_declared) {
		printer_field(printer, "members");
		printer_begin_array(printer);

		for (size_t i = 0; i < struct_def->field_count; i += 1) {
			const StructField* field = &struct_def->fields[i];
			printer_array_element(printer, i);
			printer_begin_struct(printer, "field");
			printer_string_field(printer, "name", field->name.string);
			printer_field(printer, "type");
			print_type(printer, &field->type);
			printer_end_struct(printer);
		}

		printer_end_array(printer);
	}

	printer_end_struct(printer);
}

void print_enum_def(PrinterState* printer, const Enum* enum_def) {
	assert(enum_def != NULL);

	printer_begin_struct(printer, "enum");

	printer_string_field(printer, "name", enum_def->name.string);

	printer_field(printer, "variants");
	printer_begin_array(printer);

	for (size_t i = 0; i < enum_def->variant_count; i += 1) {
		const EnumVariant* variant = &enum_def->variants[i];
		printer_array_element(printer, i);

		printer_begin_struct(printer, "variant");
		printer_string_field(printer, "name", variant->name.string);

		if (variant->value) {
			printer_field(printer, "value");
			print_expr(printer, variant->value);
		}

		printer_end_struct(printer);
	}
	printer_end_array(printer);

	printer_end_struct(printer);
}

void print_type(PrinterState* printer, const Type* type) {
	if (has_flag(type->qualifiers, TYPE_QUALIFIER_CONST)) {
		printf("const ");
	}

	switch (type->kind) {
	case PARSED_TYPE_STRUCT:
		print_struct_def(printer, type->struct_def);
		break;
	case PARSED_TYPE_UNION:
		print_struct_def(printer, type->union_def);
		break;
	case PARSED_TYPE_ENUM:
		print_enum_def(printer, type->enum_def);
		break;
	case PARSED_TYPE_VOID:
		printf("void\n");
		break;

	case PARSED_TYPE_SIZE_T:
		printf("size_t\n");
		break;

	case PARSED_TYPE_CHAR:
	case PARSED_TYPE_INT:
	case PARSED_TYPE_SHORT:
	case PARSED_TYPE_LONG:
	case PARSED_TYPE_LONG_LONG:
	case PARSED_TYPE_INT8:
	case PARSED_TYPE_INT16:
	case PARSED_TYPE_INT32:
	case PARSED_TYPE_INT64:

	case PARSED_TYPE_SIGNED_CHAR:
	case PARSED_TYPE_SIGNED_INT:
	case PARSED_TYPE_SIGNED_SHORT:
	case PARSED_TYPE_SIGNED_LONG:
	case PARSED_TYPE_SIGNED_LONG_LONG:
	case PARSED_TYPE_SIGNED_INT8:
	case PARSED_TYPE_SIGNED_INT16:
	case PARSED_TYPE_SIGNED_INT32:
	case PARSED_TYPE_SIGNED_INT64:

	case PARSED_TYPE_UNSIGNED_CHAR:
	case PARSED_TYPE_UNSIGNED_INT:
	case PARSED_TYPE_UNSIGNED_SHORT:
	case PARSED_TYPE_UNSIGNED_LONG:
	case PARSED_TYPE_UNSIGNED_LONG_LONG:
	case PARSED_TYPE_UNSIGNED_INT8:
	case PARSED_TYPE_UNSIGNED_INT16:
	case PARSED_TYPE_UNSIGNED_INT32:
	case PARSED_TYPE_UNSIGNED_INT64: {
		TypeKind base_kind = type->kind & (~(TYPE_FLAG_SIGNED | TYPE_FLAG_UNSIGNED));
		const char* prefix = "";
		const char* base_type_name = "";

		if (has_flag(type->kind, TYPE_FLAG_SIGNED)) {
			prefix = "signed ";
		} else if (has_flag(type->kind, TYPE_FLAG_UNSIGNED)) {
			prefix = "unsigned ";
		}

		switch (base_kind) {
		case PARSED_TYPE_CHAR:
			base_type_name = "char";
			break;
		case PARSED_TYPE_INT:
			base_type_name = "int";
			break;
		case PARSED_TYPE_SHORT:
			base_type_name = "short";
			break;
		case PARSED_TYPE_LONG:
			base_type_name = "long";
			break;
		case PARSED_TYPE_LONG_LONG:
			base_type_name = "long long";
			break;
		case PARSED_TYPE_INT8:
			base_type_name = "__int8";
			break;
		case PARSED_TYPE_INT16:
			base_type_name = "__int16";
			break;
		case PARSED_TYPE_INT32:
			base_type_name = "__int32";
			break;
		case PARSED_TYPE_INT64:
			base_type_name = "__int64";
			break;
		default:
			unreachable();
		}

		printf("%s%s\n", prefix, base_type_name);
		break;
	}

	case PARSED_TYPE_FLOAT:
		printf("float\n");
		break;
	case PARSED_TYPE_DOUBLE:
		printf("double\n");
		break;
	case PARSED_TYPE_POINTER:
		printer_begin_struct(printer, "pointer_type");
		printer_field(printer, "base_type");
		print_type(printer, type->pointer_base_type);
		printer_end_struct(printer);
		break;
	case PARSED_TYPE_ARRAY:
		printer_begin_struct(printer, "array_type");
		printer_field(printer, "element_type");
		print_type(printer, type->array.element_type);

		if (type->array.size) {
			printer_field(printer, "size");
			print_expr(printer, type->array.size);
		}

		printer_end_struct(printer);
		break;	break;
	}
}

void print_type_def(PrinterState* printer, const TypeDef* type_def) {
	printer_begin_struct(printer, "typedef");

	printer_field(printer, "type");
	print_type(printer, &type_def->aliased_type);
	printer_string_field(printer, "name", type_def->new_name.string);

	printer_end_struct(printer);
}

void print_scope(PrinterState* printer, const Scope* scope) {
	printer_begin_array(printer);
	printer_field(printer, "id");
	printf("%llu\n", scope->id);

	AstNode* node = scope->nodes.first;
	size_t node_index = 0;

	while (node) {
		printer_array_element(printer, node_index);
		print_single_node(printer, node);
		node = node->next;
		node_index += 1;
	}

	printer_end_array(printer);
}

void print_function_def(PrinterState* printer, const Function* function_def) {
	printer_begin_struct(printer, "function");

	if (function_def->decl_spec) {
		printer_field(printer, "decl_spec");
		print_decl_spec(printer, function_def->decl_spec);
	}

	String storage_spec_string = {};
	switch (function_def->storage_specifier) {
	case STORAGE_SPEC_NONE:
		storage_spec_string = STR_LIT("none");
		break;
	case STORAGE_SPEC_EXTERNAL:
		storage_spec_string = STR_LIT("extern");
		break;
	case STORAGE_SPEC_STATIC:
		storage_spec_string = STR_LIT("static");
		break;
	}

	printer_string_field(printer, "storage_spec", storage_spec_string);

	printer_string_field(printer, "name", function_def->name.string);
	printer_string_field(printer, "calling_convetion", function_calling_convetion_to_string(function_def->calling_convention));

	printer_field(printer, "return_type");
	print_type(printer, &function_def->return_type);

	printer_field(printer, "parameters");
	printer_begin_array(printer);

	for (size_t i = 0; i < function_def->parameter_count; i += 1) {
		const FunctionParam* param = &function_def->parameters[i];

		printer_array_element(printer, i);
		printer_begin_struct(printer, "param");

		if (param->name.string.length > 0) {
			printer_string_field(printer, "name", param->name.string);
		}

		printer_field(printer, "type");
		print_type(printer, &param->type);
		printer_end_struct(printer);
	}

	printer_end_array(printer);
	printer_bool_field(printer, "is_forward_declared", function_def->is_forward_declared);
	printer_bool_field(printer, "has_va_args", function_def->has_va_args);

	if (!function_def->is_forward_declared) {
		printer_field(printer, "body");
		if (function_def->body) {
			print_scope(printer, function_def->body);
		} else {
			printf("[]\n");
		}

	}

	printer_end_struct(printer);
}

void print_variable(PrinterState* printer, const Variable* variable) {
	printer_begin_struct(printer, "variable");
	printer_string_field(printer, "name", variable->name.string);
	printer_field(printer, "type");
	print_type(printer, &variable->type);

	if (variable->value) {
		printer_field(printer, "value");
		print_expr(printer, variable->value);
	}

	printer_end_struct(printer);
}

void print_return_stmt(PrinterState* printer, const ReturnStmt* return_stmt) {
	printer_begin_struct(printer, "return");

	if (return_stmt->value) {
		printer_field(printer, "value");
		print_expr(printer, return_stmt->value);
	}

	printer_end_struct(printer);
}

void print_single_node(PrinterState* printer, const AstNode* node) {
	switch (node->kind) {
	case AST_NODE_TYPE_DEF:
		print_type_def(printer, node->type_def);
		break;
	case AST_NODE_STRUCT:
		print_struct_def(printer, node->struct_def);
		break;
	case AST_NODE_UNION:
		print_struct_def(printer, node->union_def);
		break;
	case AST_NODE_ENUM:
		print_enum_def(printer, node->enum_def);
		break;
	case AST_NODE_FUNCTION:
		print_function_def(printer, node->function_def);
		break;
	case AST_NODE_EXPR:
		print_expr(printer, &node->expr);
		break;
	case AST_NODE_VARIABLE:
		print_variable(printer, &node->variable);
		break;
	case AST_NODE_RETURN:
		print_return_stmt(printer, &node->return_stmt);
		break;
	case AST_NODE_BLOCK:
		printer_begin_struct(printer, "block");
		printer_field(printer, "body");
		print_scope(printer, &node->block);
		printer_end_struct(printer);
		break;
	case AST_NODE_IF:
		printer_begin_struct(printer, "if");

		printer_field(printer, "condition");
		print_expr(printer, &node->if_stmt.condition);

		printer_field(printer, "true_node");
		print_single_node(printer, node->if_stmt.true_node);

		if (node->if_stmt.false_node) {
			printer_field(printer, "false_node");
			print_single_node(printer, node->if_stmt.false_node);
		}

		printer_end_struct(printer);
		break;
	}
}

void print_parsed_node(const AstNode* node) {
	PrinterState printer = {};

	while (node != NULL) {
		print_single_node(&printer, node);
		node = node->next;
	}
}
