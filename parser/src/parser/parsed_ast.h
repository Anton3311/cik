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

typedef enum {
	AST_NODE_TYPE_DEF,
	AST_NODE_STRUCT,
	AST_NODE_ENUM,
} AstNodeKind;

typedef struct {
	ParsedNode* first;
	ParsedNode* last;
	size_t count;
} ParsedNodeList;

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
// Struct
//

struct ParsedStructMember {
	SourceString name;
	ParsedType type;

	ParsedStructMember* next;
};

struct ParsedStruct {
	SourceString name;

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
// Node
//

struct ParsedNode {
	AstNodeKind kind;
	ParsedNode* next;

	union {
		ParsedStruct struct_def;
		ParsedEnum enum_def;
		ParsedTypeDef type_def;
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
