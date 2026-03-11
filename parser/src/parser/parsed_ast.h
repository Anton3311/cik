#ifndef AST_H
#define AST_H

#include "core/core.h"
#include "parser/source_info.h"

typedef String SourceString;

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
typedef struct ParsedScope ParsedScope;
typedef struct ParsedCall ParsedCall;
typedef struct ParsedExpr ParsedExpr;

//
// AST
//

typedef enum {
	AST_NODE_TYPE_DEF,
	AST_NODE_STRUCT,
	AST_NODE_ENUM,
	AST_NODE_FUNCTION,
	AST_NODE_EXPR,
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
	PARSED_TYPE_NAMED,
	PARSED_TYPE_STRUCT,
	PARSED_TYPE_ENUM,
} ParsedTypeKind;

struct ParsedType {
	SourceRange source_range;
	ParsedTypeKind kind;

	TypeQualifiers qualifiers;

	union {
		struct {
			SourceString name;
		} named;
		
		ParsedStruct* struct_def;
		ParsedEnum* enum_def;
	};
};

//
// Expr
//

struct ParsedCall {
	ParsedExpr* callable;
	size_t argument_count;
	ParsedExpr** arguments;
};

typedef enum {
	EXPR_FUNCTION_REFERENCE,
} ExprKind;

struct ParsedExpr {
	ExprKind kind;

	union {
		ParsedCall call;
		ParsedFunction* function_ref;
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
// Function
//

struct ParsedFunctionParam {
	ParsedType type;
	SourceString name;

	ParsedFunctionParam* next;
};

struct ParsedFunction {
	ParsedType return_type;
	SourceString name;

	bool is_forward_declared;
	size_t parameter_count;
	ParsedFunctionParam* parameter_list;
	ParsedScope* body;
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
		ParsedTypeDef type_def;
		ParsedFunction* function_def;
		ParsedExpr expr;
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
