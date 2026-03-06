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
typedef struct ParsedTypeDef ParsedTypeDef;

typedef enum {
	AST_NODE_TYPE_DEF,
	AST_NODE_TYPE_STRUCT,
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
	PARSED_TYPE_NAMED,
	PARSED_TYPE_STRUCT,
} ParsedTypeKind;

struct ParsedType {
	SourceRange source_range;
	ParsedTypeKind kind;

	union {
		struct {
			SourceString name;
		} named;
		
		ParsedStruct* struct_def;
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
