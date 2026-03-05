#ifndef PARSER_H
#define PARSER_H

#include "core/core.h"
#include "parser/parsed_ast.h"

// A bit unnecessary, a forward declaration would've been enough
#include "parser/tokenizer.h"
#include "parser/diagnostics.h"
#include "parser/preprocessor.h"

typedef struct {
	Arena* ast_allocator;

	Diagnostics* diagnostics;
	Preprocessor* preprocessor;
} Parser;

void parser_init(Parser* parser,
		Arena* ast_allocator,
		Preprocessor* preprocessor,
		Diagnostics* diagnostics);

void parser_parse(Parser* parser, ParsedAST* ast);

#endif
