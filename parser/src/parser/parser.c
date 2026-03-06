#include "parser.h"

bool _parser_parse_type(Parser* parser, ParsedType* out_type);

inline SourceString _source_string_from_token(Token token) {
	return token.string;
}

bool _parser_expect_semicolon(Parser* parser, String error_message) {
	Token token = preprocessor_next_token(parser->preprocessor);
	if (token.kind != TOKEN_SEMICOLON) {
		TokenKind expected_tokens[] = { TOKEN_SEMICOLON };
		diagnostics_report_unexpected_token(parser->diagnostics,
				token,
				expected_tokens,
				array_size(expected_tokens));
		return false;
	}

	return true;
}

#define try(expression) if (!(expression)) { return 0; }

bool _parser_parse_struct_members(Parser* parser, size_t* out_member_count, ParsedStructMember** out_members) {
	assert(out_member_count != NULL);
	assert(out_members != NULL);

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

bool _parser_parse_enum_variants(Parser* parser, size_t* out_variant_count, ParsedEnumVariant** out_variant_list) {
	assert(out_variant_count != NULL);
	assert(out_variant_list != NULL);

	Token left_brace = preprocessor_next_token(parser->preprocessor);
	assert(left_brace.kind == TOKEN_LEFT_BRACE);

	ArenaRegion temp = arena_begin_temp(parser->ast_allocator);

	ParsedEnumVariant* first_variant = NULL;
	ParsedEnumVariant* last_variant = NULL;
	size_t variant_count = 0;

	while (true) {
		{
			Token token = preprocessor_view_next(parser->preprocessor);
			if (token.kind == TOKEN_RIGHT_BRACE) {
				preprocessor_next_token(parser->preprocessor);
				break;
			}
		}

		ParsedEnumVariant* variant = arena_alloc(parser->ast_allocator, ParsedEnumVariant);
		memset(variant, 0, sizeof(*variant));

		Token name_token = preprocessor_next_token(parser->preprocessor);
		if (name_token.kind != TOKEN_IDENT) {
			TokenKind expected_tokens[] = { TOKEN_IDENT };
			diagnostics_report_unexpected_token(parser->diagnostics,
					name_token,
					expected_tokens,
					array_size(expected_tokens));
			arena_end_temp(temp);
			return false;
		}

		variant->name = _source_string_from_token(name_token);

		if (first_variant == NULL) {
			first_variant = variant;
			last_variant = variant;
		} else {
			last_variant->next = variant;
			last_variant = variant;
		}

		variant_count += 1;

		Token comma_or_right_brace = preprocessor_view_next(parser->preprocessor);
		if (comma_or_right_brace.kind == TOKEN_RIGHT_BRACE) {
			preprocessor_next_token(parser->preprocessor); // consume TOKEN_RIGHT_BRACE
			break;
		} else if (comma_or_right_brace.kind == TOKEN_COMMA) {
			preprocessor_next_token(parser->preprocessor); // consume TOKEN_COMMA
			continue;
		} else {
			TokenKind expected_tokens[] = { TOKEN_RIGHT_BRACE, TOKEN_COMMA };
			diagnostics_report_unexpected_token(parser->diagnostics,
					name_token,
					expected_tokens,
					array_size(expected_tokens));
			arena_end_temp(temp);
			return false;
		}
	}

	*out_variant_count = variant_count;
	*out_variant_list = first_variant;
	return true;
}

bool _parser_parse_enum_type(Parser* parser, ParsedEnum* out_enum_def) {
	assert(out_enum_def != NULL);

	Token keyword_token = preprocessor_next_token(parser->preprocessor);
	assert(keyword_token.kind == TOKEN_KEYWORD_ENUM);

	SourceString enum_name = {};
	ParsedEnumVariant* variant_list = {};
	size_t variant_count = 0;

	Token token = preprocessor_view_next(parser->preprocessor);
	if (token.kind == TOKEN_IDENT) {
		preprocessor_next_token(parser->preprocessor); // consume identifier

		enum_name = _source_string_from_token(token);
		token = preprocessor_view_next(parser->preprocessor);
	}

	if (token.kind == TOKEN_LEFT_BRACE) {
		if (!_parser_parse_enum_variants(parser, &variant_count, &variant_list)) {
			return false;
		}
	}

	if (variant_list != NULL) {
		assert(variant_count > 0);
	}

	*out_enum_def = (ParsedEnum) {
		.name = enum_name,
		.variant_list = variant_list,
		.variant_count = variant_count
	};
	return true;
}

bool _parser_parse_type(Parser* parser, ParsedType* out_type) {
	assert(out_type != NULL);

	// First parse qualifiers
	while (true) {
		Token token = preprocessor_view_next(parser->preprocessor);
		if (token.kind == TOKEN_KEYWORD_CONST) {
			preprocessor_next_token(parser->preprocessor);
			out_type->qualifiers |= TYPE_QUALIFIER_CONST;
		} else {
			break;
		}
	}

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
	} else if (token.kind == TOKEN_KEYWORD_ENUM) {
		ParsedEnum enum_def = {};
		if (!_parser_parse_enum_type(parser, &enum_def)) {
			return false;
		}

		out_type->kind = PARSED_TYPE_ENUM;
		out_type->enum_def = arena_alloc(parser->ast_allocator, ParsedEnum);
		*out_type->enum_def = enum_def;
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

ParsedNode* _parser_parse_type_def(Parser* parser) {
	Token keyword_token = preprocessor_next_token(parser->preprocessor);
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

bool _parser_parse_function_param_list(Parser* parser, ParsedFunctionParam** out_param_list, size_t* out_param_count) {
	Token left_paren = preprocessor_next_token(parser->preprocessor);
	assert(left_paren.kind == TOKEN_LEFT_PAREN);

	ArenaRegion temp = arena_begin_temp(parser->ast_allocator);

	ParsedFunctionParam* first_param = NULL;
	ParsedFunctionParam* last_param = NULL;
	size_t param_count = 0;

	while (true) {
		{
			Token token = preprocessor_view_next(parser->preprocessor);
			if (token.kind == TOKEN_RIGHT_PAREN) {
				preprocessor_next_token(parser->preprocessor);
				break;
			}
		}

		ParsedFunctionParam* param = arena_alloc(parser->ast_allocator, ParsedFunctionParam);

		if (!_parser_parse_type(parser, &param->type)) {
			arena_end_temp(temp);
			return false;
		}

		Token token = preprocessor_view_next(parser->preprocessor);
		if (token.kind == TOKEN_IDENT) {
			preprocessor_next_token(parser->preprocessor); // consume param name

			param->name = _source_string_from_token(token);
			token = preprocessor_view_next(parser->preprocessor);
		}

		if (first_param == NULL) {
			first_param = param;
			last_param = param;
		} else {
			last_param->next = param;
			last_param = param;
		}

		param_count += 1;

		if (token.kind == TOKEN_COMMA) {
			preprocessor_next_token(parser->preprocessor);
			continue; // we most likely have more parameters
		} else if (token.kind == TOKEN_RIGHT_PAREN) {
			preprocessor_next_token(parser->preprocessor);
			break;
		}
	}

	if (first_param != NULL) {
		assert(param_count > 0);
	}

	*out_param_list = first_param;
	*out_param_count = param_count;
	return true;
}

bool _parser_parse_variable_or_function_def(Parser* parser, ParsedNode* out_node) {
	assert(out_node != NULL);

	ArenaRegion temp = arena_begin_temp(parser->ast_allocator);

	ParsedType type = {};
	SourceString name = {};

	if (!_parser_parse_type(parser, &type)) {
		arena_end_temp(temp);
		return false;
	}

	Token name_token = preprocessor_next_token(parser->preprocessor);
	if (name_token.kind != TOKEN_IDENT) {
		arena_end_temp(temp);

		TokenKind expected_tokens[] = { TOKEN_IDENT };
		diagnostics_report_unexpected_token(parser->diagnostics,
				name_token,
				expected_tokens,
				array_size(expected_tokens));
		return false;
	}

	name = _source_string_from_token(name_token);

	Token token = preprocessor_view_next(parser->preprocessor);
	if (token.kind == TOKEN_LEFT_PAREN) {
		ParsedFunctionParam* param_list = NULL;
		size_t param_count = 0;

		if (!_parser_parse_function_param_list(parser, &param_list, &param_count)) {
			arena_end_temp(temp);
			return false;
		}

		try(_parser_expect_semicolon(parser, STR_LIT("Expected ';' at the end of the function definition")));

		out_node->kind = AST_NODE_FUNCTION;
		out_node->function_def = (ParsedFunction) {
			.return_type = type,
			.name = name,
			.parameter_list = param_list,
			.parameter_count = param_count,
			.body = NULL,
		};

		return true;
	} else {
		TokenKind expected_tokens[] = { TOKEN_LEFT_PAREN };
		diagnostics_report_unexpected_token(parser->diagnostics,
				token,
				expected_tokens,
				array_size(expected_tokens));
		return false;
	}

	unreachable();
	return false;
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
		Token token = preprocessor_view_next(parser->preprocessor);
		if (token.kind == TOKEN_EOF) {
			break;
		}

		switch (token.kind) {
		case TOKEN_KEYWORD_TYPEDEF: {
			ParsedNode* node = _parser_parse_type_def(parser);
			if (node) {
				parsed_node_list_append(&ast->root_nodes, node);
			}
			break;
		}
		case TOKEN_KEYWORD_ENUM: {
			ArenaRegion temp = arena_begin_temp(parser->ast_allocator);
			ParsedNode* node = arena_alloc(parser->ast_allocator, ParsedNode);
			node->kind = AST_NODE_ENUM;

			if (!_parser_parse_enum_type(parser, &node->enum_def)) {
				arena_end_temp(temp);
				break;
			}

			Token semicolon = preprocessor_next_token(parser->preprocessor);
			if (semicolon.kind != TOKEN_SEMICOLON) {
				arena_end_temp(temp);

				TokenKind expected_tokens[] = { TOKEN_SEMICOLON };
				diagnostics_report_unexpected_token(parser->diagnostics,
						semicolon,
						expected_tokens,
						array_size(expected_tokens));
				break;
			}

			parsed_node_list_append(&ast->root_nodes, node);
			break;
		}
		default: {
			ArenaRegion temp = arena_begin_temp(parser->ast_allocator);
			ParsedNode* node = arena_alloc(parser->ast_allocator, ParsedNode);
			memset(node, 0, sizeof(*node));

			if (_parser_parse_variable_or_function_def(parser, node)) {
				parsed_node_list_append(&ast->root_nodes, node);
				break;
			} else {
				arena_end_temp(temp);
			}

			preprocessor_next_token(parser->preprocessor);
			diagnostics_report_unexpected_token(parser->diagnostics, token, NULL, 0);
			break;
		}
		}
	}
}
