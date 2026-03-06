#include "parser.h"

bool _parser_parse_type(Parser* parser, ParsedType* out_type);

inline SourceString _source_string_from_token(Token token) {
	return token.string;
}

bool _parser_parse_struct_members(Parser* parser, size_t* out_member_count, ParsedStructMember** out_members) {
	assert(out_member_count != NULL);

	Token left_brace = preprocessor_next_token(parser->preprocessor);
	assert(left_brace.kind == TOKEN_LEFT_BRACE);

	ArenaRegion temp = arena_begin_temp(parser->ast_allocator);

	ParsedStructMember* first_member = NULL;
	ParsedStructMember* last_member = NULL;
	size_t member_count = 0;

	while (true) {
		{
			Token token = preprocessor_view_next(parser->preprocessor);
			if (token.kind == TOKEN_RIGHT_BRACE) {
				preprocessor_next_token(parser->preprocessor);
				break;
			}
		}

		ParsedStructMember* member = arena_alloc(parser->ast_allocator, ParsedStructMember);
		memset(member, 0, sizeof(*member));

		if (!_parser_parse_type(parser, &member->type)) {
			arena_end_temp(temp);
			return false;
		}

		Token name_or_semilcolon = preprocessor_view_next(parser->preprocessor);
		if (name_or_semilcolon.kind == TOKEN_IDENT) {
			member->name = _source_string_from_token(name_or_semilcolon);
			preprocessor_next_token(parser->preprocessor); // consume name

			name_or_semilcolon = preprocessor_view_next(parser->preprocessor);
		}

		if (name_or_semilcolon.kind == TOKEN_SEMICOLON) {
			// consume semicolon
			preprocessor_next_token(parser->preprocessor);
		} else {
			TokenKind expected_tokens[] = { TOKEN_SEMICOLON };
			diagnostics_report_unexpected_token(parser->diagnostics,
					name_or_semilcolon,
					expected_tokens,
					array_size(expected_tokens));
			arena_end_temp(temp);
			return false;
		}

		if (first_member == NULL) {
			first_member = member;
			last_member = member;
		} else {
			last_member->next = member;
			last_member = member;
		}

		member_count += 1;
	}

	*out_member_count = member_count;
	*out_members = first_member;
	return true;
}

bool _parser_parse_struct_type(Parser* parser, ParsedStruct* out_struct_def) {
	assert(out_struct_def != NULL);

	Token keyword_token = preprocessor_next_token(parser->preprocessor);
	assert(keyword_token.kind == TOKEN_KEYWORD_STRUCT);

	SourceString struct_name = {};
	ParsedStructMember* member_list = NULL;
	size_t member_count = 0;

	Token token = preprocessor_view_next(parser->preprocessor);
	if (token.kind == TOKEN_IDENT) {
		preprocessor_next_token(parser->preprocessor); // consume identifier

		struct_name = _source_string_from_token(token);
		token = preprocessor_view_next(parser->preprocessor);
	}

	if (token.kind == TOKEN_LEFT_BRACE) {
		if (!_parser_parse_struct_members(parser, &member_count, &member_list)) {
			return false;
		}
	}

	if (member_list != NULL) {
		assert(member_count > 0);
	}

	*out_struct_def = (ParsedStruct) {
		.name = struct_name,
		.member_list = member_list,
		.member_count = member_count,
	};

	return true;
}

bool _parser_parse_type(Parser* parser, ParsedType* out_type) {
	assert(out_type != NULL);

	Token token = preprocessor_view_next(parser->preprocessor);

	if (token.kind == TOKEN_IDENT) {
		preprocessor_next_token(parser->preprocessor);

		out_type->kind = PARSED_TYPE_NAMED;
		out_type->source_range = token.source_range;
		out_type->named.name = _source_string_from_token(token);
		return true;
	} else if (token.kind == TOKEN_KEYWORD_STRUCT) {
		ParsedStruct struct_def = {};
		if (!_parser_parse_struct_type(parser, &struct_def)) {
			return false;
		}

		out_type->kind = PARSED_TYPE_STRUCT;
		out_type->struct_def = arena_alloc(parser->ast_allocator, ParsedStruct);
		*out_type->struct_def = struct_def;
		return true;
	} else {
		preprocessor_next_token(parser->preprocessor);

		TokenKind expected_tokens[] = {
			TOKEN_IDENT,
			TOKEN_KEYWORD_STRUCT,
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
