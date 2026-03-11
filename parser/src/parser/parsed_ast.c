#include "parsed_ast.h"

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

void print_struct_def(PrinterState* printer, const ParsedStruct* struct_def) {
	assert(struct_def != NULL);

	printer_begin_struct(printer, "struct");
	printer_string_field(printer, "name", struct_def->name);
	printer_bool_field(printer, "is_forward_declared", struct_def->is_forward_declared);
	
	if (!struct_def->is_forward_declared) {
		printer_field(printer, "members");
		printer_begin_array(printer);

		size_t member_index = 0;
		ParsedStructMember* member = struct_def->member_list;
		while (member) {
			printer_array_element(printer, member_index);
			printer_begin_struct(printer, "member");
			printer_string_field(printer, "name", member->name);
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

	printer_string_field(printer, "name", enum_def->name);

	printer_field(printer, "variants");
	printer_begin_array(printer);

	size_t variant_index = 0;
	ParsedEnumVariant* variant = enum_def->variant_list;
	while (variant != NULL) {
		printer_array_element(printer, variant_index);

		printer_begin_struct(printer, "variant");
		printer_string_field(printer, "name", variant->name);
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
	case PARSED_TYPE_NAMED:
		printer_string_value(printer, type->named.name);
		break;
	case PARSED_TYPE_STRUCT:
		print_struct_def(printer, type->struct_def);
		break;
	case PARSED_TYPE_ENUM:
		print_enum_def(printer, type->enum_def);
		break;
	}
}

void print_type_def(PrinterState* printer, const ParsedTypeDef* type_def) {
	printer_begin_struct(printer, "typedef");

	printer_field(printer, "type");
	print_type(printer, &type_def->aliased_type);
	printer_string_field(printer, "name", type_def->new_name);

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

	printer_string_field(printer, "name", function_def->name);
	printer_field(printer, "return_type");
	print_type(printer, &function_def->return_type);

	printer_field(printer, "parameters");
	printer_begin_array(printer);

	ParsedFunctionParam* param = function_def->parameter_list;
	size_t param_index = 0;
	while (param != NULL) {
		printer_array_element(printer, param_index);
		printer_begin_struct(printer, "param");
		printer_string_field(printer, "name", param->name);
		printer_field(printer, "type");
		print_type(printer, &param->type);
		printer_end_struct(printer);

		param = param->next;
		param_index += 1;
	}

	printer_end_array(printer);
	printer_bool_field(printer, "is_forward_declared", function_def->is_forward_declared);

	if (function_def->is_forward_declared) {
		printer_field(printer, "body");
		if (function_def->body) {
			print_scope(printer, function_def->body);
		} else {
			printf("[]\n");
		}

	}

	printer_end_struct(printer);
}

void print_single_node(PrinterState* printer, const ParsedNode* node) {
	switch (node->kind) {
	case AST_NODE_TYPE_DEF:
		print_type_def(printer, &node->type_def);
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
	}
}

void print_parsed_node(const ParsedNode* node) {
	PrinterState printer = {};

	while (node != NULL) {
		print_single_node(&printer, node);
		node = node->next;
	}
}
