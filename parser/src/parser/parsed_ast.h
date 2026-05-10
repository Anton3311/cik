#ifndef AST_H
#define AST_H

#include "core/core.h"
#include "parser/source_info.h"
#include "parser/parse_tools.h"

typedef struct ParsedNode ParsedNode;
typedef struct ParsedBlock ParsedBlock;
typedef struct ParsedType ParsedType;
typedef struct ParsedStruct ParsedStruct;
typedef struct ParsedStructField ParsedStructField;
typedef struct ParsedStructFieldNamespace ParsedStructFieldNamespace;
typedef struct ParsedStructFieldNamespaceEntry ParsedStructFieldNamespaceEntry;
typedef struct ParsedEnum ParsedEnum;
typedef struct ParsedEnumVariant ParsedEnumVariant;
typedef struct ParsedTypeDef ParsedTypeDef;
typedef struct ParsedFunction ParsedFunction;
typedef struct ParsedFunctionParam ParsedFunctionParam;
typedef struct ParsedVariable ParsedVariable;
typedef struct ParsedScope ParsedScope;
typedef struct ParsedCall ParsedCall;
typedef struct ParsedStringLiteral ParsedStringLiteral;
typedef struct ParsedExpr ParsedExpr;
typedef struct ParsedExprArray ParsedExprArray;
typedef struct ParsedBinExpr ParsedBinExpr;
typedef struct ParsedUnaryExpr ParsedUnaryExpr;
typedef struct ParsedIntegerLiteral ParsedIntegerLiteral;
typedef struct ParsedReturnStmt ParsedReturnStmt;
typedef struct ParsedDeclSpec ParsedDeclSpec;
typedef struct ParsedIfStmt ParsedIfStmt;

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
	ParsedNode* first;
	ParsedNode* last;
	size_t count;
} ParsedNodeList;

struct ParsedScope {
	ParsedNodeList nodes;
};

struct ParsedExprArray {
	ParsedExpr** exprs;
	size_t count;
};

void parsed_node_list_append(ParsedNodeList* list, ParsedNode* node);

struct ParsedBlock {
	ParsedNodeList nodes;
};

typedef enum {
	TYPE_QUALIFIER_NONE = 0,
	TYPE_QUALIFIER_CONST = 1 << 0,
} TypeQualifiers;

typedef enum {
	TYPE_FLAG_NONE     = 0,
	TYPE_FLAG_SIGNED   = 1 << 8,
	TYPE_FLAG_UNSIGNED = 2 << 8,
} ParsedTypeKindFlags;

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
} ParsedTypeKind;

struct ParsedType {
	SourceRange source_range;
	ParsedTypeKind kind;

	TypeQualifiers qualifiers;
	ParsedTypeDef* alias_definition;

	union {
		struct {
			SourceString name;
		} named;
		
		ParsedStruct* struct_def;
		ParsedStruct* union_def;
		ParsedEnum* enum_def;
		ParsedType* pointer_base_type;

		struct {
			ParsedType* element_type;
			ParsedExpr* size;
		} array;
	};
};

bool type_is_struct(const ParsedType* type, const ParsedStruct* struct_def);

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

struct ParsedBinExpr {
	BinOpKind op;
	ParsedExpr* left;
	ParsedExpr* right;
};

struct ParsedUnaryExpr {
	UnaryOpKind op;
	ParsedExpr* operand;
};

struct ParsedCall {
	ParsedExpr* callable;
	ParsedExprArray args;
};

struct ParsedStringLiteral {
	String full_string;
};

struct ParsedIntegerLiteral {
	SourceRange source_range;
	IntergerLiteralFormat format;
	ParsedTypeKind integer_type;
	uint64_t value;
};

typedef enum {
	EXPR_CALL,
	EXPR_BINARY,
	EXPR_UNARY,
	EXPR_FUNCTION_REFERENCE,
	EXPR_VARIABLE_REFERENCE,
	EXPR_INTEGER_LITERAL,
	EXPR_STRING_LITERAL,
	EXPR_ENUM_CONSTANT,
	EXPR_FUNCTION_PARAM,
} ExprKind;

struct ParsedExpr {
	ExprKind kind;

	union {
		ParsedCall call;
		ParsedFunction* function_ref;
		ParsedVariable* variable_ref;
		ParsedBinExpr binary;
		ParsedUnaryExpr unary;
		ParsedIntegerLiteral int_literal;
		ParsedStringLiteral string_literal;
		
		struct {
			const ParsedEnum* enum_def;
			size_t variant_index;
		} enum_constant;

		struct {
			const ParsedFunction* function_def;
			size_t param_index;
		} function_param;
	};
};

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
struct ParsedStructFieldNamespaceEntry {
	const ParsedStruct* struct_def;
	size_t field_index;
};

struct ParsedStructFieldNamespace {
	String* keys;
	ParsedStructFieldNamespaceEntry* entries;
	size_t size;
	size_t capacity;
};

// Performs a lookup in the hashmap and returns the index of the found entry, or SIZE_MAX if not found
size_t struct_field_namespace_index_of(const ParsedStructFieldNamespace* struct_namespace, String name);

struct ParsedStructField {
	SourceString name;
	ParsedType type;
};

typedef enum {
	STRUCT_LAYOUT_KIND_STRUCT,
	STRUCT_LAYOUT_KIND_UNION,
} StructLayoutKind;

struct ParsedStruct {
	SourceString name;
	StructLayoutKind layout_kind;

	bool is_forward_declared;

	ParsedStructField* fields;
	size_t field_count;

	ParsedStructFieldNamespace* field_namespace;
};

//
// Enum
//

struct ParsedEnumVariant {
	SourceString name;
	ParsedExpr* value;
};

struct ParsedEnum {
	SourceString name;

	bool is_forward_declared;

	ParsedEnumVariant* variants;
	size_t variant_count;
};

//
// TypeDef
//

struct ParsedTypeDef {
	ParsedType aliased_type;
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

struct ParsedDeclSpec {
	DeclSpecKind kind;

	union {
		ParsedStringLiteral deprecation_text;
	};

	ParsedDeclSpec* next;
};

//
// Function
//

typedef enum {
	FUNC_CALL_CONV_DEFAULT,
	FUNC_CALL_CONV_CDECL,
} FunctionCallingConvention;

String function_calling_convetion_to_string(FunctionCallingConvention conv);

struct ParsedFunctionParam {
	ParsedType type;
	SourceString name;
};

struct ParsedFunction {
	ParsedType return_type;
	SourceString name;

	bool is_inline;
	bool is_forward_declared;
	bool has_va_args;
	FunctionCallingConvention calling_convention;
	size_t parameter_count;
	ParsedFunctionParam* parameters;
	ParsedScope* body;
	ParsedDeclSpec* decl_spec;
	StorageSpecifier storage_specifier;
	uint32_t var_count;
};

//
// Variable
//

struct ParsedVariable {
	SourceString name;
	ParsedType type;
	ParsedExpr* value;
	StorageSpecifier storage_specifier;
	uint32_t id;
};

//
// ReturnStmt
//

struct ParsedReturnStmt {
	ParsedExpr* value;
};

//
// IfStmt
//

struct ParsedIfStmt {
	ParsedExpr condition;
	ParsedNode* true_node;

	// This one can be optional
	ParsedNode* false_node;
};

//
// Node
//

struct ParsedNode {
	AstNodeKind kind;
	ParsedNode* next;

	union {
		ParsedStruct* struct_def;
		ParsedStruct* union_def;
		ParsedEnum* enum_def;
		ParsedTypeDef* type_def;
		ParsedFunction* function_def;
		ParsedExpr expr;
		ParsedVariable variable;
		ParsedReturnStmt return_stmt;
		ParsedScope block;
		ParsedIfStmt if_stmt;
	};
};

//
// AST
//

typedef struct {
	ParsedNodeList root_nodes;
} ParsedAST;

void print_parsed_node(const ParsedNode* node);

#endif
