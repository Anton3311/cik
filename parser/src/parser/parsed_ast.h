#ifndef AST_H
#define AST_H

#include "core/core.h"
#include "parser/source_info.h"
#include "parser/parse_tools.h"

typedef struct ParsedNode ParsedNode;
typedef struct ParsedBlock ParsedBlock;
typedef struct ParsedType ParsedType;
typedef struct ParsedStruct ParsedStruct;
typedef struct ParsedStructMember ParsedStructMember;
typedef struct ParsedEnum ParsedEnum;
typedef struct ParsedEnumVariant ParsedEnumVariant;
typedef struct ParsedTypeDef ParsedTypeDef;
typedef struct ParsedFunction ParsedFunction;
typedef struct ParsedFunctionParam ParsedFunctionParam;
typedef struct ParsedVariable ParsedVariable;
typedef struct ParsedScope ParsedScope;
typedef struct ParsedCall ParsedCall;
typedef struct ParsedExpr ParsedExpr;
typedef struct ParsedBinExpr ParsedBinExpr;
typedef struct ParsedUnaryExpr ParsedUnaryExpr;
typedef struct ParsedIntegerLiteral ParsedIntegerLiteral;
typedef struct ParsedReturnStmt ParsedReturnStmt;
typedef struct ParsedDeclSpec ParsedDeclSpec;

//
// AST
//

typedef enum {
	AST_NODE_TYPE_DEF,
	AST_NODE_STRUCT,
	AST_NODE_ENUM,
	AST_NODE_FUNCTION,
	AST_NODE_EXPR,
	AST_NODE_VARIABLE,
	AST_NODE_RETURN,
} AstNodeKind;

typedef struct {
	ParsedNode* first;
	ParsedNode* last;
	size_t count;
} ParsedNodeList;

struct ParsedScope {
	ParsedNodeList nodes;
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
	PARSED_TYPE_ENUM               = 14,

	PARSED_TYPE_POINTER            = 15,
	PARSED_TYPE_ARRAY              = 16,
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
		ParsedEnum* enum_def;
		ParsedType* pointer_base_type;

		struct {
			ParsedType* element_type;
			ParsedExpr* size;
		} array;
	};
};

//
// Expr
//

typedef enum {
	BIN_OP_ADD,
	BIN_OP_SUB,
	BIN_OP_MUL,
	BIN_OP_DIV,
	BIN_OP_MOD,
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
	size_t argument_count;
	ParsedExpr** arguments;
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
	};
};

//
// Struct
//

struct ParsedStructMember {
	SourceString name;
	ParsedType type;

	ParsedStructMember* next;
};

struct ParsedStruct {
	SourceString name;

	bool is_forward_declared;

	ParsedStructMember* member_list;
	size_t member_count;
};

//
// Enum
//

struct ParsedEnumVariant {
	SourceString name;

	ParsedEnumVariant* next;
};

struct ParsedEnum {
	SourceString name;

	bool is_forward_declared;

	ParsedEnumVariant* variant_list;
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

	ParsedFunctionParam* next;
};

struct ParsedFunction {
	ParsedType return_type;
	SourceString name;

	bool is_forward_declared;
	FunctionCallingConvention calling_convention;
	size_t parameter_count;
	ParsedFunctionParam* parameter_list;
	ParsedScope* body;
	ParsedDeclSpec* decl_spec;
};

//
// Variable
//

struct ParsedVariable {
	SourceString name;
	ParsedType type;
	ParsedExpr* value;
};

//
// ReturnStmt
//

struct ParsedReturnStmt {
	ParsedExpr* value;
};

//
// Node
//

struct ParsedNode {
	AstNodeKind kind;
	ParsedNode* next;

	union {
		ParsedStruct* struct_def;
		ParsedEnum* enum_def;
		ParsedTypeDef* type_def;
		ParsedFunction* function_def;
		ParsedExpr expr;
		ParsedVariable variable;
		ParsedReturnStmt return_stmt;
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
