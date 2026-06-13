#ifndef AST_H
#define AST_H

#include "core/core.h"
#include "parser/source_info.h"
#include "parser/parse_tools.h"

typedef struct AstNode AstNode;
typedef struct Block Block;
typedef struct Type Type;
typedef struct Struct Struct;
typedef struct StructField StructField;
typedef struct StructFieldNamespace StructFieldNamespace;
typedef struct StructFieldNamespaceEntry StructFieldNamespaceEntry;
typedef struct Enum Enum;
typedef struct EnumVariant EnumVariant;
typedef struct TypeDef TypeDef;
typedef struct Function Function;
typedef struct FunctionParam FunctionParam;
typedef struct Variable Variable;
typedef struct Scope Scope;
typedef struct Call Call;
typedef struct StringLiteral StringLiteral;
typedef struct CharLiteral CharLiteral;
typedef struct Expr Expr;
typedef struct ExprArray ExprArray;
typedef struct BinExpr BinExpr;
typedef struct UnaryExpr UnaryExpr;
typedef struct IntegerLiteral IntegerLiteral;
typedef struct ReturnStmt ReturnStmt;
typedef struct DeclSpec DeclSpec;
typedef struct IfStmt IfStmt;
typedef struct ArrayIndex ArrayIndex;

//
// AST
//

typedef enum {
	AST_NODE_TYPE_DEF,
	AST_NODE_STRUCT,
	AST_NODE_UNION,
	AST_NODE_ENUM,
	AST_NODE_FUNCTION,
	AST_NODE_EXPR,
	AST_NODE_VARIABLE,
	AST_NODE_RETURN,
	AST_NODE_BLOCK,
	AST_NODE_IF,
} AstNodeKind;

typedef enum {
	STORAGE_SPEC_NONE,
	STORAGE_SPEC_EXTERNAL,
	STORAGE_SPEC_STATIC,
} StorageSpecifier;

typedef struct {
	AstNode* first;
	AstNode* last;
	size_t count;
} NodeList;

struct Scope {
	uint64_t id;
	NodeList nodes;
};

struct ExprArray {
	Expr** exprs;
	size_t count;
};

void parsed_node_list_append(NodeList* list, AstNode* node);

struct Block {
	NodeList nodes;
};

typedef enum {
	TYPE_QUALIFIER_NONE = 0,
	TYPE_QUALIFIER_CONST = 1 << 0,
} TypeQualifiers;

typedef enum {
	TYPE_FLAG_NONE     = 0,
	TYPE_FLAG_SIGNED   = 1 << 8,
	TYPE_FLAG_UNSIGNED = 2 << 8,
} TypeKindFlags;

typedef enum {
	PARSED_TYPE_VOID               = 0,

	PARSED_TYPE_CHAR               = 1,
	PARSED_TYPE_INT                = 2,
	PARSED_TYPE_SHORT              = 3,
	PARSED_TYPE_LONG               = 4,
	PARSED_TYPE_LONG_LONG          = 5,
	PARSED_TYPE_INT8               = 6,
	PARSED_TYPE_INT16              = 7,
	PARSED_TYPE_INT32              = 8,
	PARSED_TYPE_INT64              = 9,

	PARSED_TYPE_SIGNED_CHAR        = 1 | TYPE_FLAG_SIGNED,
	PARSED_TYPE_SIGNED_INT         = 2 | TYPE_FLAG_SIGNED,
	PARSED_TYPE_SIGNED_SHORT       = 3 | TYPE_FLAG_SIGNED,
	PARSED_TYPE_SIGNED_LONG        = 4 | TYPE_FLAG_SIGNED,
	PARSED_TYPE_SIGNED_LONG_LONG   = 5 | TYPE_FLAG_SIGNED,
	PARSED_TYPE_SIGNED_INT8        = 6 | TYPE_FLAG_SIGNED,
	PARSED_TYPE_SIGNED_INT16       = 7 | TYPE_FLAG_SIGNED,
	PARSED_TYPE_SIGNED_INT32       = 8 | TYPE_FLAG_SIGNED,
	PARSED_TYPE_SIGNED_INT64       = 9 | TYPE_FLAG_SIGNED,

	PARSED_TYPE_UNSIGNED_CHAR      = 1 | TYPE_FLAG_UNSIGNED,
	PARSED_TYPE_UNSIGNED_INT       = 2 | TYPE_FLAG_UNSIGNED,
	PARSED_TYPE_UNSIGNED_SHORT     = 3 | TYPE_FLAG_UNSIGNED,
	PARSED_TYPE_UNSIGNED_LONG      = 4 | TYPE_FLAG_UNSIGNED,
	PARSED_TYPE_UNSIGNED_LONG_LONG = 5 | TYPE_FLAG_UNSIGNED,
	PARSED_TYPE_UNSIGNED_INT8      = 6 | TYPE_FLAG_UNSIGNED,
	PARSED_TYPE_UNSIGNED_INT16     = 7 | TYPE_FLAG_UNSIGNED,
	PARSED_TYPE_UNSIGNED_INT32     = 8 | TYPE_FLAG_UNSIGNED,
	PARSED_TYPE_UNSIGNED_INT64     = 9 | TYPE_FLAG_UNSIGNED,
	
	PARSED_TYPE_SIZE_T             = 10,

	PARSED_TYPE_FLOAT              = 11,
	PARSED_TYPE_DOUBLE             = 12,

	PARSED_TYPE_STRUCT             = 13,
	PARSED_TYPE_UNION              = 14,
	PARSED_TYPE_ENUM               = 15,

	PARSED_TYPE_POINTER            = 16,
	PARSED_TYPE_ARRAY              = 17,
} TypeKind;

struct Type {
	TypeKind kind;

	TypeQualifiers qualifiers;
	TypeDef* alias_definition;

	union {
		Struct* struct_def;
		Struct* union_def;
		Enum* enum_def;
		Type* pointer_base_type;

		struct {
			Type* element_type;
			Expr* size;
		} array;
	};
};

bool type_equal(const Type* a, const Type* b);
void type_array_to_pointer(const Type* type, Type* out_type);

inline bool type_kind_is_int(TypeKind kind) {
	TypeKind kind_without_sign_flags = kind & ~(TYPE_FLAG_SIGNED | TYPE_FLAG_UNSIGNED);
	return (kind_without_sign_flags >= PARSED_TYPE_CHAR
		&& kind_without_sign_flags <= PARSED_TYPE_INT64)
		|| kind == PARSED_TYPE_SIZE_T;
}

uint32_t type_get_int_convertion_rank(const Type* type);

inline bool type_kind_is_pointer_like(TypeKind kind) {
	return kind == PARSED_TYPE_POINTER || kind == PARSED_TYPE_ARRAY;
}

inline Type* type_extract_pointer_base_type(Type* type) {
	switch (type->kind) {
	case PARSED_TYPE_POINTER:
		return type->pointer_base_type;
	case PARSED_TYPE_ARRAY:
		return type->array.element_type;
	default:
		break;
	}

	unreachable();
	return NULL;
}

bool type_is_struct(const Type* type, const Struct* struct_def);
bool type_is_enum(const Type* type, const Enum* enum_def);

//
// Expr
//

typedef enum {
	BIN_OP_ADD,
	BIN_OP_SUB,
	BIN_OP_MUL,
	BIN_OP_DIV,
	BIN_OP_MOD,

	BIN_OP_LOGICAL_AND,
	BIN_OP_LOGICAL_OR,

	BIN_OP_LOGICAL_EQUAL,
	BIN_OP_LOGICAL_NOT_EQUAL,
	BIN_OP_LOGICAL_LESS,
	BIN_OP_LOGICAL_GREATER,
	BIN_OP_LOGICAL_LESS_OR_EQUAL,
	BIN_OP_LOGICAL_GREATER_OR_EQUAL,

	BIN_OP_BITWISE_AND,
	BIN_OP_BITWISE_OR,
	BIN_OP_BITWISE_XOR,
	BIN_OP_BITWISE_SHIFT_LEFT,
	BIN_OP_BITWISE_SHIFT_RIGHT,

	BIN_OP_ASSIGNMENT,

	BIN_OP_ASSIGNMENT_BY_SUM,
	BIN_OP_ASSIGNMENT_BY_DIFFERENCE,
	BIN_OP_ASSIGNMENT_BY_PRODUCT,
	BIN_OP_ASSIGNMENT_BY_QUOTIENT,
	BIN_OP_ASSIGNMENT_BY_REMAINDER,

	BIN_OP_ASSIGNMENT_BY_BITWISE_AND,
	BIN_OP_ASSIGNMENT_BY_BITWISE_OR,
	BIN_OP_ASSIGNMENT_BY_BITWISE_XOR,
	BIN_OP_ASSIGNMENT_BY_BITWISE_SHIFT_LEFT,
	BIN_OP_ASSIGNMENT_BY_BITWISE_SHIFT_RIGHT,
} BinOpKind;

inline bool bin_op_is_compare(BinOpKind kind) {
	return kind >= BIN_OP_LOGICAL_EQUAL && kind <= BIN_OP_LOGICAL_GREATER_OR_EQUAL;
}

typedef enum {
	UNARY_OP_NEGATE,
	UNARY_OP_PLUS,
	UNARY_OP_ADDRESS,
	UNARY_OP_DEREFERENCE,

	UNARY_OP_LOGICAL_NOT,

	UNARY_OP_BITWISE_NOT,

	UNARY_OP_PRE_INCREMENT,
	UNARY_OP_POST_INCREMENT,

	UNARY_OP_PRE_DECREMENT,
	UNARY_OP_POST_DECREMENT,
} UnaryOpKind;

String bin_op_kind_to_string(BinOpKind op);
String unary_op_kind_to_string(UnaryOpKind op);
uint32_t bin_op_precedence(BinOpKind op);

struct BinExpr {
	BinOpKind op;

	// `TypeKind` and `pointer_base_type` is enough to
	// represent all possible arithmetic types, since binary
	// operations are only supported by arithmetic types 
	TypeKind result_type_kind;
	Type* pointer_base_type;

	Expr* left;
	Expr* right;
};

void bin_expr_select_result_type(
		const Type* left_type,
		const Type* right_type,
		Type* out_type);

struct UnaryExpr {
	UnaryOpKind op;
	Expr* operand;
};

struct Call {
	Expr* callable;
	ExprArray args;
};

struct StringLiteral {
	String full_string;
};

// TODO: When implmenenting wide char support,
//       can just add a EXPR_WIDE_CHAR_LITERAL,
//       instead of adding a flag here.
struct CharLiteral {
	uint32_t value;
};

struct IntegerLiteral {
	IntergerLiteralFormat format;
	TypeKind integer_type;
	uint64_t value;
};

struct ArrayIndex {
	Expr* array;
	Expr* index;
};

typedef enum {
	EXPR_CALL,
	EXPR_BINARY,
	EXPR_UNARY,
	EXPR_FUNCTION_REFERENCE,
	EXPR_VARIABLE_REFERENCE,
	EXPR_INTEGER_LITERAL,
	EXPR_STRING_LITERAL,
	EXPR_CHAR_LITERAL,
	EXPR_ENUM_CONSTANT,
	EXPR_FUNCTION_PARAM,
	EXPR_ARRAY_INDEX,
} ExprKind;

struct Expr {
	ExprKind kind;

	union {
		Call call;
		Function* function_ref;
		Variable* variable_ref;
		BinExpr binary;
		UnaryExpr unary;
		IntegerLiteral int_literal;
		StringLiteral string_literal;
		CharLiteral char_literal;
		ArrayIndex array_index;
		
		struct {
			const Enum* enum_def;
			size_t variant_index;
		} enum_constant;

		struct {
			const Function* function_def;
			size_t param_index;
		} function_param;
	};
};

void expr_get_type(Expr* expr, Type* out_type);

//
// Struct
//

// `StructFieldNamespace` is a hash map that is used to map the field name
// to an actual field of this or an inner anonymous struct.
//
// Example:
//
// struct Nested {
//     struct Inner1 {
//         int a;
//         
//         struct Inner2 {
//             int inner_most_value;
//         };
//     };
//
//     String text;
// };
//
// For the above struct the next expressions are valid:
// 1. nested.text;
// 2. nested.a;
// 3. nested.inner_most_value;
//
// Thus the hash map would contains other next mappings:
// 1. text             -> Nested.text
// 2. a                -> Nested.Inner1.a
// 3. inner_most_value -> Nested.Inner1.Inner2.inner_most_value
struct StructFieldNamespaceEntry {
	const Struct* struct_def;
	size_t field_index;
};

struct StructFieldNamespace {
	String* keys;
	StructFieldNamespaceEntry* entries;
	size_t size;
	size_t capacity;
};

// Performs a lookup in the hashmap and returns the index of the found entry, or SIZE_MAX if not found
size_t struct_field_namespace_index_of(const StructFieldNamespace* struct_namespace, String name);

struct StructField {
	SourceString name;
	Type type;
};

typedef enum {
	STRUCT_LAYOUT_KIND_STRUCT,
	STRUCT_LAYOUT_KIND_UNION,
} StructLayoutKind;

struct Struct {
	SourceString name;
	StructLayoutKind layout_kind;

	bool is_forward_declared;

	StructField* fields;
	size_t field_count;

	StructFieldNamespace* field_namespace;
};

inline const StructField* struct_find_field(const Struct* struct_def, String field_name) {
	assert(struct_def->field_namespace);
	size_t entry_index = struct_field_namespace_index_of(struct_def->field_namespace, field_name);
	if (entry_index == SIZE_MAX) {
		return NULL;
	}

	const StructFieldNamespaceEntry entry = struct_def->field_namespace->entries[entry_index];
	return &entry.struct_def->fields[entry.field_index];
}

//
// Enum
//

struct EnumVariant {
	SourceString name;
	Expr* value;
};

struct Enum {
	SourceString name;

	bool is_forward_declared;

	EnumVariant* variants;
	size_t variant_count;
};

//
// TypeDef
//

struct TypeDef {
	Type aliased_type;
	SourceString new_name;
};

//
// DeclSpec
// 

typedef enum {
	DECL_SPEC_DEPRECATED,
	DECL_SPEC_NO_INLINE,
	DECL_SPEC_NO_RETURN,
	DECL_SPEC_DLL_IMPORT,
	DECL_SPEC_DLL_EXPORT,
	DECL_SPEC_RESTRICT,
} DeclSpecKind;

struct DeclSpec {
	DeclSpecKind kind;

	union {
		StringLiteral deprecation_text;
	};

	DeclSpec* next;
};

//
// Function
//

typedef enum {
	FUNC_CALL_CONV_DEFAULT,
	FUNC_CALL_CONV_CDECL,
} FunctionCallingConvention;

String function_calling_convetion_to_string(FunctionCallingConvention conv);

struct FunctionParam {
	Type type;
	SourceString name;
};

struct Function {
	Type return_type;
	SourceString name;

	bool is_inline;
	bool is_forward_declared;
	bool has_va_args;
	FunctionCallingConvention calling_convention;
	size_t parameter_count;
	FunctionParam* parameters;
	Scope* body;
	DeclSpec* decl_spec;
	StorageSpecifier storage_specifier;
	uint32_t var_count;
};

//
// Variable
//

struct Variable {
	SourceString name;
	Type type;
	Expr* value;
	StorageSpecifier storage_specifier;
	uint32_t id;
};

//
// ReturnStmt
//

struct ReturnStmt {
	Expr* value;
};

//
// IfStmt
//

struct IfStmt {
	Expr condition;
	AstNode* true_node;

	// This one is optional
	AstNode* false_node;
};

//
// Node
//

struct AstNode {
	AstNodeKind kind;
	AstNode* next;

	Scope* parent_scope;

	union {
		Struct* struct_def;
		Struct* union_def;
		Enum* enum_def;
		TypeDef* type_def;
		Function* function_def;
		Expr expr;
		Variable variable;
		ReturnStmt return_stmt;
		Scope block;
		IfStmt if_stmt;
	};
};

//
// AST
//

typedef struct {
	NodeList root_nodes;
} AST;

void print_parsed_node(const AstNode* node);

#endif
