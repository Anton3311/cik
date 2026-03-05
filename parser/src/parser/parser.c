#include "parser.h"

inline SourceString _source_string_from_token(Token token) {
	return token.string;
}

bool _parser_parse_type(Parser* parser, ParsedType* out_type) {
	assert(out_type != NULL);

	Token token = preprocessor_next_token(parser->preprocessor);

	if (token.kind == TOKEN_IDENT) {
		out_type->source_range = token.source_range;
		out_type->named.name = _source_string_from_token(token);

		return true;
	} else {
		TokenKind expected_tokens[] = {
			TOKEN_IDENT,
		};

		diagnostics_report_unexpected_token(parser->diagnostics,
				token,
				expected_tokens,
				array_size(expected_tokens));
		return false;
	}

	unreachable();
	return false;
}

ParsedNode* _parser_parse_type_def(Parser* parser, Token keyword_token) {
	assert(keyword_token.kind == TOKEN_KEYWORD_TYPEDEF);

	ParsedType aliased_type = {};
	if (!_parser_parse_type(parser, &aliased_type)) {
		return NULL;
	}

	Token new_name = preprocessor_next_token(parser->preprocessor);
	if (new_name.kind != TOKEN_IDENT) {
		TokenKind expected_tokens[] = {
			TOKEN_IDENT,
		};

		diagnostics_report_unexpected_token(parser->diagnostics,
				new_name,
				expected_tokens,
				array_size(expected_tokens));
		return NULL;
	}

	Token semicolon = preprocessor_next_token(parser->preprocessor);
	if (semicolon.kind != TOKEN_SEMICOLON) {
		TokenKind expected_tokens[] = {
			TOKEN_SEMICOLON,
		};

		diagnostics_report_unexpected_token(parser->diagnostics,
				semicolon,
				expected_tokens,
				array_size(expected_tokens));
		return NULL;
	}

	ParsedNode* node = arena_alloc(parser->ast_allocator, ParsedNode);
	node->kind = AST_NODE_TYPE_DEF;
	node->type_def = (ParsedTypeDef) {
		.aliased_type = aliased_type,
		.new_name = new_name.string,
	};

	return node;
}

void parser_init(Parser* parser,
		Arena* ast_allocator,
		Preprocessor* preprocessor,
		Diagnostics* diagnostics) {
	parser->ast_allocator = ast_allocator;
	parser->diagnostics = diagnostics;
	parser->preprocessor = preprocessor;
}

void parser_parse(Parser* parser, ParsedAST* ast) {
	ast->root_nodes = (ParsedNodeList) {};

	while (true) {
		Token token = preprocessor_next_token(parser->preprocessor);
		if (token.kind == TOKEN_EOF) {
			break;
		}

		switch (token.kind) {
		case TOKEN_KEYWORD_TYPEDEF: {
			ParsedNode* node = _parser_parse_type_def(parser, token);
			if (node) {
				parsed_node_list_append(&ast->root_nodes, node);
			}
			break;
		}
		default: {
			diagnostics_report_unexpected_token(parser->diagnostics, token, NULL, 0);
			break;
		}
		}
	}
}
