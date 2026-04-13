#include "parsed_ast.h"

String bin_op_kind_to_string(BinOpKind op) {
	switch (op) {
	case BIN_OP_ADD:
		return STR_LIT("+");
	case BIN_OP_SUB:
		return STR_LIT("-");
	case BIN_OP_MUL:
		return STR_LIT("*");
	case BIN_OP_DIV:
		return STR_LIT("/");
	case BIN_OP_MOD:
		return STR_LIT("%");
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
	}

	unreachable();
	return UINT32_MAX;
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

void parsed_node_list_append(ParsedNodeList* list, ParsedNode* node) {
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

void print_type(PrinterState* printer, const ParsedType* type);
void print_single_node(PrinterState* printer, const ParsedNode* node);

void print_expr(PrinterState* printer, const ParsedExpr* expr) {
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
	case EXPR_BINARY:
		printer_begin_struct(printer, "binary_expr");
		printer_string_field(printer, "kind", bin_op_kind_to_string(expr->binary.op));
		printer_field(printer, "left");
		print_expr(printer, expr->binary.left);
		printer_field(printer, "right");
		print_expr(printer, expr->binary.right);
		printer_end_struct(printer);
		break;
	case EXPR_INTEGER_LITERAL:
		printer_begin_struct(printer, "int_literal");
		printer_string_field(printer, "format", int_literal_format_to_string(expr->int_literal.format));
		printer_field(printer, "type");

		ParsedType type = { .kind = expr->int_literal.integer_type };
		print_type(printer, &type);
		printer_field(printer, "value");
		printf("%llu\n", expr->int_literal.value);
		printer_end_struct(printer);
		break;
	}
}

void print_struct_def(PrinterState* printer, const ParsedStruct* struct_def) {
	assert(struct_def != NULL);

	printer_begin_struct(printer, "struct");
	printer_string_field(printer, "name", struct_def->name.string);
	printer_bool_field(printer, "is_forward_declared", struct_def->is_forward_declared);
	
	if (!struct_def->is_forward_declared) {
		printer_field(printer, "members");
		printer_begin_array(printer);

		size_t member_index = 0;
		ParsedStructMember* member = struct_def->member_list;
		while (member) {
			printer_array_element(printer, member_index);
			printer_begin_struct(printer, "member");
			printer_string_field(printer, "name", member->name.string);
			printer_field(printer, "type");
			print_type(printer, &member->type);
			printer_end_struct(printer);

			member = member->next;
			member_index += 1;
		}

		printer_end_array(printer);
	}

	printer_end_struct(printer);
}

void print_enum_def(PrinterState* printer, const ParsedEnum* enum_def) {
	assert(enum_def != NULL);

	printer_begin_struct(printer, "enum");

	printer_string_field(printer, "name", enum_def->name.string);

	printer_field(printer, "variants");
	printer_begin_array(printer);

	size_t variant_index = 0;
	ParsedEnumVariant* variant = enum_def->variant_list;
	while (variant != NULL) {
		printer_array_element(printer, variant_index);

		printer_begin_struct(printer, "variant");
		printer_string_field(printer, "name", variant->name.string);
		printer_end_struct(printer);

		variant = variant->next;
		variant_index += 1;
	}
	printer_end_array(printer);

	printer_end_struct(printer);
}

void print_type(PrinterState* printer, const ParsedType* type) {
	if (has_flag(type->qualifiers, TYPE_QUALIFIER_CONST)) {
		printf("const ");
	}

	switch (type->kind) {
	case PARSED_TYPE_STRUCT:
		print_struct_def(printer, type->struct_def);
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

	case PARSED_TYPE_SIGNED_CHAR:
	case PARSED_TYPE_SIGNED_INT:
	case PARSED_TYPE_SIGNED_SHORT:
	case PARSED_TYPE_SIGNED_LONG:
	case PARSED_TYPE_SIGNED_LONG_LONG:

	case PARSED_TYPE_UNSIGNED_CHAR:
	case PARSED_TYPE_UNSIGNED_INT:
	case PARSED_TYPE_UNSIGNED_SHORT:
	case PARSED_TYPE_UNSIGNED_LONG:
	case PARSED_TYPE_UNSIGNED_LONG_LONG: {
		ParsedTypeKind base_kind = type->kind & (~(TYPE_FLAG_SIGNED | TYPE_FLAG_UNSIGNED));
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

void print_type_def(PrinterState* printer, const ParsedTypeDef* type_def) {
	printer_begin_struct(printer, "typedef");

	printer_field(printer, "type");
	print_type(printer, &type_def->aliased_type);
	printer_string_field(printer, "name", type_def->new_name.string);

	printer_end_struct(printer);
}

void print_scope(PrinterState* printer, const ParsedScope* scope) {
	printer_begin_array(printer);

	ParsedNode* node = scope->nodes.first;
	size_t node_index = 0;

	while (node) {
		printer_array_element(printer, node_index);
		print_single_node(printer, node);
		node = node->next;
		node_index += 1;
	}

	printer_end_array(printer);
}

void print_function_def(PrinterState* printer, const ParsedFunction* function_def) {
	printer_begin_struct(printer, "function");

	printer_string_field(printer, "name", function_def->name.string);
	printer_string_field(printer, "calling_convetion", function_calling_convetion_to_string(function_def->calling_convention));

	printer_field(printer, "return_type");
	print_type(printer, &function_def->return_type);

	printer_field(printer, "parameters");
	printer_begin_array(printer);

	ParsedFunctionParam* param = function_def->parameter_list;
	size_t param_index = 0;
	while (param != NULL) {
		printer_array_element(printer, param_index);
		printer_begin_struct(printer, "param");
		printer_string_field(printer, "name", param->name.string);
		printer_field(printer, "type");
		print_type(printer, &param->type);
		printer_end_struct(printer);

		param = param->next;
		param_index += 1;
	}

	printer_end_array(printer);
	printer_bool_field(printer, "is_forward_declared", function_def->is_forward_declared);

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

void print_variable(PrinterState* printer, const ParsedVariable* variable) {
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

void print_return_stmt(PrinterState* printer, const ParsedReturnStmt* return_stmt) {
	printer_begin_struct(printer, "return");

	if (return_stmt->value) {
		printer_field(printer, "value");
		print_expr(printer, return_stmt->value);
	}

	printer_end_struct(printer);
}

void print_single_node(PrinterState* printer, const ParsedNode* node) {
	switch (node->kind) {
	case AST_NODE_TYPE_DEF:
		print_type_def(printer, node->type_def);
		break;
	case AST_NODE_STRUCT:
		print_struct_def(printer, node->struct_def);
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
	}
}

void print_parsed_node(const ParsedNode* node) {
	PrinterState printer = {};

	while (node != NULL) {
		print_single_node(&printer, node);
		node = node->next;
	}
}
