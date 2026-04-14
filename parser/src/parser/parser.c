#include "parser.h"

#include "parser/parse_tools.h"

bool type_equal(const ParsedType* a, const ParsedType* b) {
	ParsedTypeKind a_without_signed = a->kind & (~TYPE_FLAG_SIGNED);
	ParsedTypeKind b_without_signed = b->kind & (~TYPE_FLAG_SIGNED);

	if (a_without_signed != b_without_signed) {
		return false;
	}

	if (a->qualifiers != b->qualifiers) {
		return false;
	}

	switch (a->kind) {
	case PARSED_TYPE_STRUCT:
		return a->struct_def == b->struct_def;
	case PARSED_TYPE_ENUM:
		return a->enum_def == b->enum_def;
	case PARSED_TYPE_VOID:
	case PARSED_TYPE_SIZE_T:

	case PARSED_TYPE_CHAR:
	case PARSED_TYPE_INT:
	case PARSED_TYPE_SHORT:
	case PARSED_TYPE_LONG:
	case PARSED_TYPE_LONG_LONG:

	case PARSED_TYPE_SIGNED_CHAR:
	case PARSED_TYPE_SIGNED_INT:
	case PARSED_TYPE_SIGNED_SHORT:
	case PARSED_TYPE_SIGNED_LONG:
	case PARSED_TYPE_SIGNED_LONG_LONG:

	case PARSED_TYPE_UNSIGNED_CHAR:
	case PARSED_TYPE_UNSIGNED_INT:
	case PARSED_TYPE_UNSIGNED_SHORT:
	case PARSED_TYPE_UNSIGNED_LONG:
	case PARSED_TYPE_UNSIGNED_LONG_LONG:

	case PARSED_TYPE_FLOAT:
	case PARSED_TYPE_DOUBLE:
		return true;
	
	case PARSED_TYPE_POINTER:
		assert(a->pointer_base_type != NULL);
		assert(b->pointer_base_type != NULL);
		return type_equal(a->pointer_base_type, b->pointer_base_type);
	
	case PARSED_TYPE_ARRAY:
		assert(a->array.size == NULL);
		assert(b->array.size == NULL);
		return type_equal(a->array.element_type, a->array.element_type);
	}

	unreachable();
	return false;
}

//
// IdentifierStorage
//

const void* REMOVED_SLOT_FLAG = (void*)0x1;

inline IdentifierNamespace* _ident_storage_get_namespace(IdentifierStorage* storage,
		IdentifierNamespaceKind namespace_kind) {

	assert(namespace_kind >= 0);
	assert(namespace_kind < IDENT_NAMESPACE_COUNT);
	return &storage->namespaces[namespace_kind];
}

inline bool _ident_storage_remove_entry_at(IdentifierNamespace* ident_namespace, size_t index) {
	assert(ident_namespace->count > 0);

	assert(index < ident_namespace->capacity);

	assert_msg(ident_namespace->keys[index].v != NULL, "Cannot remove empty slot");
	assert_msg(ident_namespace->keys[index].v != REMOVED_SLOT_FLAG, "Cannot remove already removed slot");

	ident_namespace->keys[index] = (String) { .v = REMOVED_SLOT_FLAG, .length = 0 };
	ident_namespace->count -= 1;
	return true;
}

static size_t _ident_storage_try_find_entry(IdentifierNamespace* ident_namespace, String name) {
	assert(ident_namespace != NULL);
	assert(name.length > 0);

	size_t entry_index = hash_string(name) % ident_namespace->capacity;
	while (true) {
		String key = ident_namespace->keys[entry_index];
		if (key.v == NULL) {
			return SIZE_MAX;
		}

		if (str_equal(key, name)) {
			return entry_index;
		}

		entry_index = (entry_index + 1) % ident_namespace->capacity;
	}

	return SIZE_MAX;
}

static size_t _ident_storage_try_find_empty_entry(IdentifierNamespace* ident_namespace, String name) {
	assert(ident_namespace != NULL);
	assert(name.length > 0);

	size_t entry_index = hash_string(name) % ident_namespace->capacity;
	while (true) {
		String key = ident_namespace->keys[entry_index];
		if (key.v == NULL || key.v == REMOVED_SLOT_FLAG) {
			return entry_index;
		}

		entry_index = (entry_index + 1) % ident_namespace->capacity;
	}

	return SIZE_MAX;
}

inline void _ident_storage_alloc_namespace_hash_map(IdentifierNamespace* ident_namespace) {
	size_t key_size = sizeof(String);
	size_t entry_size = sizeof(IdentifierEntry*);
	size_t buffer_size = (key_size + entry_size) * ident_namespace->capacity;
	uint8_t* new_buffer = heap_alloc_bytes(buffer_size);

	memset(new_buffer, 0, buffer_size);

	ident_namespace->keys = (String*)new_buffer;
	ident_namespace->entries = (IdentifierEntry**)(new_buffer + key_size * ident_namespace->capacity);
}

static void _ident_storage_grow_namespace(IdentifierNamespace* ident_namespace) {
	assert(ident_namespace);

	if (ident_namespace->count == 0) {
		assert(ident_namespace->keys == NULL);
		assert(ident_namespace->entries == NULL);
	}

	size_t old_capacity = ident_namespace->capacity;
	String* old_keys = ident_namespace->keys;
	IdentifierEntry** old_entries = ident_namespace->entries;

	ident_namespace->capacity = ident_namespace->capacity + ident_namespace->capacity / 2;
	_ident_storage_alloc_namespace_hash_map(ident_namespace);

	for (size_t i = 0; i < old_capacity; i += 1) {
		if (old_keys[i].v == NULL || old_keys[i].v == REMOVED_SLOT_FLAG) {
			continue;
		}

		size_t empty_slot = _ident_storage_try_find_empty_entry(ident_namespace, old_keys[i]);
		assert(empty_slot != SIZE_MAX);

		ident_namespace->keys[empty_slot] = old_keys[i];
		ident_namespace->entries[empty_slot] = old_entries[i];
	}
}

inline bool _ident_storage_try_insert(IdentifierNamespace* ident_namespace, String name, IdentifierEntry* entry) {
	if (ident_namespace->count + 1 == ident_namespace->capacity / 2) {
		_ident_storage_grow_namespace(ident_namespace);
	}
	
	size_t empty_slot = _ident_storage_try_find_empty_entry(ident_namespace, name);
	if (empty_slot == SIZE_MAX) {
		return false;
	}

	ident_namespace->keys[empty_slot] = name;
	ident_namespace->entries[empty_slot] = entry;
	ident_namespace->count += 1;
	return true;
}

inline IdentifierEntry* ident_storage_find(IdentifierStorage* storage,
		IdentifierNamespaceKind namespace_kind,
		String name) {

	IdentifierNamespace* ident_namespace = _ident_storage_get_namespace(storage, namespace_kind);
	size_t index = _ident_storage_try_find_entry(ident_namespace, name);
	return index == SIZE_MAX ? NULL : ident_namespace->entries[index];
}

void _ident_storage_init_namespace(IdentifierNamespace* ident_namespace) {
	ident_namespace->count = 0;
	ident_namespace->capacity = 64;

	_ident_storage_alloc_namespace_hash_map(ident_namespace);
}

void ident_storage_init(IdentifierStorage* storage, Arena* allocator) {
	assert(storage != NULL);

	storage->allocator = allocator;

	for (size_t i = 0; i < IDENT_NAMESPACE_COUNT; i += 1) {
		_ident_storage_init_namespace(&storage->namespaces[i]);
	}

	storage->current_scope = arena_alloc(storage->allocator, IdentifierScope);
	memset(storage->current_scope, 0, sizeof(*storage->current_scope));

	storage->next_scope_id = 1;
	storage->next_free_entry = NULL;
	storage->next_free_scope = NULL;
}

void ident_storage_release(IdentifierStorage* storage) {
	for (size_t i = 0; i < IDENT_NAMESPACE_COUNT; i += 1) {
		IdentifierNamespace* ident_namespace = &storage->namespaces[i];

		if (ident_namespace->count > 0) {
			assert(ident_namespace->keys);
			assert(ident_namespace->entries);

			heap_release(ident_namespace->keys);

			ident_namespace->keys = NULL;
			ident_namespace->entries = NULL;
		}
	}
	
}

IdentifierEntry* ident_storage_insert(IdentifierStorage* storage,
		IdentifierNamespaceKind namespace_kind,
		SourceString name) {

	assert(storage != NULL);
	assert(name.string.length > 0);
	assert(storage->current_scope);

	IdentifierNamespace* ident_namespace = _ident_storage_get_namespace(storage, namespace_kind);

	IdentifierEntry* entry = NULL;

	size_t existing_entry_index = _ident_storage_try_find_entry(ident_namespace, name.string);
	if (existing_entry_index == SIZE_MAX) {
		assert(ident_namespace->count < ident_namespace->capacity);

		if (storage->next_free_entry) {
			// Reuse a free entry
			entry = storage->next_free_entry;
			storage->next_free_entry = storage->next_free_entry->next_in_scope;
		} else {
			entry = arena_alloc(storage->allocator, IdentifierEntry);
		}

		memset(entry, 0, sizeof(*entry));

		bool entry_inserted = _ident_storage_try_insert(ident_namespace, name.string, entry);
		assert(entry_inserted);
	} else {
		assert_msg(ident_namespace->entries[existing_entry_index]->owner_scope != storage->current_scope,
				"Identifier with the given name is already defined in the current scope");

		entry = arena_alloc(storage->allocator, IdentifierEntry);
		memset(entry, 0, sizeof(*entry));

		entry->prev = ident_namespace->entries[existing_entry_index];
		ident_namespace->entries[existing_entry_index] = entry;
	}

	entry->name = name;
	entry->owner_namespace = namespace_kind;
	entry->owner_scope = storage->current_scope;

	// Add to the current scope
	if (storage->current_scope->first_identifier == NULL) {
		storage->current_scope->first_identifier = entry;
	} else {
		storage->current_scope->last_identifier->next_in_scope = entry;
	}

	storage->current_scope->last_identifier = entry;

	assert(entry != NULL);
	return entry;
}

void ident_storage_remove(IdentifierStorage* storage,
		IdentifierNamespaceKind namespace_kind,
		SourceString name) {

	assert(storage != NULL);
	assert(name.string.length > 0);
	assert(storage->current_scope);

	IdentifierNamespace* ident_namespace = _ident_storage_get_namespace(storage, namespace_kind);

	size_t existing_entry_index = _ident_storage_try_find_entry(ident_namespace, name.string);
	if (existing_entry_index == SIZE_MAX) {
		return;
	}

	IdentifierEntry* entry = ident_namespace->entries[existing_entry_index];
	memset(entry, 0, sizeof(*entry));

	if (entry->next_in_scope == NULL) {
		_ident_storage_remove_entry_at(ident_namespace, existing_entry_index);
	} else {
		ident_namespace->entries[existing_entry_index] = entry->next_in_scope;
	}

	entry->next_in_scope = storage->next_free_entry;
}

IdentifierScope* ident_storage_begin_scope(IdentifierStorage* storage) {
	assert(storage != NULL);
	assert(storage->current_scope != NULL);

	IdentifierScope* scope;
	if (storage->next_free_scope == NULL) {
		scope = arena_alloc(storage->allocator, IdentifierScope);
	} else {
		scope = storage->next_free_scope;
		storage->next_free_scope = storage->next_free_scope->next_free;
	}

	memset(scope, 0, sizeof(*scope));

	scope->id = storage->next_scope_id;
	storage->next_scope_id += 1;

	scope->parent = storage->current_scope;
	storage->current_scope = scope;
	return scope;
}

void ident_storage_end_scope(IdentifierStorage* storage) {
	assert(storage->current_scope);

	IdentifierScope* scope = storage->current_scope;
	for (IdentifierEntry* entry = scope->first_identifier; entry != NULL;) {
		IdentifierEntry* next_entry = entry->next_in_scope;

		ident_storage_remove(storage, entry->owner_namespace, entry->name);

		entry = next_entry;
	}

	IdentifierScope* parent_scope = scope->parent;

	scope->next_free = storage->next_free_scope;
	storage->next_free_scope = scope;

	storage->current_scope = parent_scope;
}

//
// Parser
//

bool _parser_parse_type(Parser* parser, ParsedType* out_type);
bool _parser_parse_scope(Parser* parser, ParsedScope* out_scope);
bool _parser_parse_pre_declaration_modifiers(Parser* parser,
		ParsedType* base_type,
		ParsedType* out_type,
		bool duplicate_base_type);

bool _parser_parse_post_declaration_modifiers(Parser* parser,
		ParsedType* base_type,
		ParsedType* out_type,
		bool duplicate_base_type);



void _parser_skip_until_semicolon(Parser* parser) {
	while (true) {
		Token token = preprocessor_next_token(parser->preprocessor);
		if (token.kind == TOKEN_SEMICOLON) {
			break;
		} else if (token.kind == TOKEN_EOF) {
			break;
		}
	}
}

inline bool _parser_try_consume_token(Parser* parser, TokenKind expected_kind) {
	Token token = preprocessor_view_next(parser->preprocessor);
	if (token.kind == expected_kind) {
		preprocessor_next_token(parser->preprocessor);
		return true;
	}

	return false;
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

		if (!_parser_parse_pre_declaration_modifiers(parser, &member->type, &member->type, true)) {
			arena_end_temp(temp);
			return false;
		}

		Token name_or_semilcolon = preprocessor_view_next(parser->preprocessor);
		if (name_or_semilcolon.kind == TOKEN_IDENT) {
			member->name = source_string_from_token(name_or_semilcolon);
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

bool _parser_parse_struct_def(Parser* parser, ParsedStruct** out_struct_def) {
	assert(out_struct_def != NULL);

	Token keyword_token = preprocessor_next_token(parser->preprocessor);
	assert(keyword_token.kind == TOKEN_KEYWORD_STRUCT);

	SourceString struct_name = {};
	ParsedStructMember* member_list = NULL;
	size_t member_count = 0;
	bool is_forward_declared = true;

	Token token = preprocessor_view_next(parser->preprocessor);
	if (token.kind == TOKEN_IDENT) {
		preprocessor_next_token(parser->preprocessor); // consume identifier

		struct_name = source_string_from_token(token);
		token = preprocessor_view_next(parser->preprocessor);
	}

	// TODO: handle the case when there is no TOKEN_IDENT after the keyword

	if (token.kind == TOKEN_LEFT_BRACE) {
		is_forward_declared = false;
		if (!_parser_parse_struct_members(parser, &member_count, &member_list)) {
			return false;
		}
	}

	if (member_list != NULL) {
		assert(member_count > 0);
	}

	bool struct_def_initialized = false;
	ParsedStruct* struct_def = NULL;
	if (struct_name.string.length == 0) {
		struct_def = arena_alloc(parser->ast_allocator, ParsedStruct);
		memset(struct_def, 0, sizeof(*struct_def));

		struct_def_initialized = false;
	} else {
		IdentifierEntry* entry = ident_storage_find(&parser->ident_storage, IDENT_NAMESPACE_TAGGED, struct_name.string);
		if (entry) {
			if (!has_flag(entry->kind, IDENT_STRUCT)) {
				StringBuilder builder = { .arena = parser->diagnostics->allocator };
				str_builder_append_char(&builder, '\'');
				str_builder_append(&builder, entry->name.string);
				str_builder_append(&builder, STR_LIT("' is previously defined with a different tag type"));

				DiagnosticsEntry* error = diagnostics_report_error(parser->diagnostics,
						source_string_to_range(struct_name),
						builder.string,
						NULL);

				diagnostics_report_error(parser->diagnostics,
						source_string_to_range(struct_name),
						STR_LIT("Previously defined here"),
						error);
				return false;
			}
			
			struct_def = entry->struct_def;
			assert(struct_def);

			struct_def_initialized = true;

			if (!struct_def->is_forward_declared && !is_forward_declared) {
				StringBuilder builder = { .arena = parser->diagnostics->allocator };
				str_builder_append(&builder, STR_LIT("Redefinition of '"));
				str_builder_append(&builder, entry->name.string);
				str_builder_append_char(&builder, '\'');

				DiagnosticsEntry* error = diagnostics_report_error(parser->diagnostics,
						source_string_to_range(struct_name),
						builder.string,
						NULL);

				diagnostics_report_error(parser->diagnostics,
						source_string_to_range(entry->name),
						STR_LIT("Previously defined here"),
						error);
				return false;
			}
		} else {
			entry = ident_storage_insert(&parser->ident_storage, IDENT_NAMESPACE_TAGGED, struct_name);
			entry->kind = IDENT_STRUCT;

			struct_def = arena_alloc(parser->ast_allocator, ParsedStruct);
			memset(struct_def, 0, sizeof(*struct_def));

			entry->struct_def = struct_def;
		}
	}

	assert(struct_def);

	if (!struct_def_initialized) {
		struct_def->name = struct_name;
		struct_def->is_forward_declared = is_forward_declared;
	}

	if (is_forward_declared) {
		assert(member_count == 0);
		assert(member_list == NULL);
	} else {
		struct_def->member_count = member_count;
		struct_def->member_list = member_list;
		struct_def->is_forward_declared = false;
	}

	*out_struct_def = struct_def;
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

		variant->name = source_string_from_token(name_token);

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

bool _parser_parse_enum_def(Parser* parser, ParsedEnum** out_enum_def) {
	assert(out_enum_def != NULL);

	Token keyword_token = preprocessor_next_token(parser->preprocessor);
	assert(keyword_token.kind == TOKEN_KEYWORD_ENUM);

	SourceString enum_name = {};
	ParsedEnumVariant* variant_list = {};
	size_t variant_count = 0;
	bool is_forward_declared = true;

	Token token = preprocessor_view_next(parser->preprocessor);
	if (token.kind == TOKEN_IDENT) {
		preprocessor_next_token(parser->preprocessor); // consume identifier

		enum_name = source_string_from_token(token);
		token = preprocessor_view_next(parser->preprocessor);
	}

	if (token.kind == TOKEN_LEFT_BRACE) {
		is_forward_declared = false;
		if (!_parser_parse_enum_variants(parser, &variant_count, &variant_list)) {
			return false;
		}
	}

	if (variant_list != NULL) {
		assert(variant_count > 0);
	}

	ParsedEnum* enum_def = NULL;
	if (enum_name.string.length > 0) {
		IdentifierEntry* entry = ident_storage_find(&parser->ident_storage, IDENT_NAMESPACE_TAGGED, enum_name.string);

		if (entry) {
			if (!has_flag(entry->kind, IDENT_ENUM)) {
				StringBuilder builder = { .arena = parser->diagnostics->allocator };
				str_builder_append_char(&builder, '\'');
				str_builder_append(&builder, entry->name.string);
				str_builder_append(&builder, STR_LIT("' is previously defined with a different tag type"));

				DiagnosticsEntry* error = diagnostics_report_error(parser->diagnostics,
						source_string_to_range(enum_name),
						builder.string,
						NULL);

				diagnostics_report_error(parser->diagnostics,
						source_string_to_range(entry->name),
						STR_LIT("Previously defined here"),
						error);
				return false;
			}

			enum_def = entry->enum_def;
			assert(enum_def);

			if (!enum_def->is_forward_declared && !is_forward_declared) {
				StringBuilder builder = { .arena = parser->diagnostics->allocator };
				str_builder_append(&builder, STR_LIT("Redefinition of '"));
				str_builder_append(&builder, entry->name.string);
				str_builder_append_char(&builder, '\'');

				DiagnosticsEntry* error = diagnostics_report_error(parser->diagnostics,
						source_string_to_range(enum_name),
						builder.string,
						NULL);

				diagnostics_report_error(parser->diagnostics,
						source_string_to_range(entry->name),
						STR_LIT("Previously defined here"),
						error);
				return false;
			}
		} else {
			entry = ident_storage_insert(&parser->ident_storage, IDENT_NAMESPACE_TAGGED, enum_name);
			entry->kind = IDENT_ENUM;

			enum_def = arena_alloc(parser->ast_allocator, ParsedEnum);
			memset(enum_def, 0, sizeof(*enum_def));
			
			enum_def->name = enum_name;
			enum_def->is_forward_declared = is_forward_declared;

			entry->enum_def = enum_def;
		}
	} else {
		enum_def = arena_alloc(parser->ast_allocator, ParsedEnum);
		memset(enum_def, 0, sizeof(*enum_def));

		enum_def->name = enum_name;
		enum_def->is_forward_declared = is_forward_declared;
	}

	assert(enum_def);

	if (is_forward_declared) {
		assert(variant_count == 0);
		assert(variant_list == NULL);
	} else {
		enum_def->variant_count = variant_count;
		enum_def->variant_list = variant_list;
		enum_def->is_forward_declared = false;
	}

	*out_enum_def = enum_def;
	return true;
}

TypeQualifiers _parser_parse_type_qualifiers(Parser* parser) {
	TypeQualifiers qualifiers = TYPE_QUALIFIER_NONE;
	while (true) {
		Token token = preprocessor_view_next(parser->preprocessor);
		if (token.kind == TOKEN_KEYWORD_CONST) {
			preprocessor_next_token(parser->preprocessor);
			qualifiers |= TYPE_QUALIFIER_CONST;
		} else {
			break;
		}
	}

	return qualifiers;
}

typedef enum {
	TYPE_OR_EXPR_ERROR,
	TYPE_OR_EXPR_PARSED_TYPE,
	TYPE_OR_EXPR_PARSED_EXPR,
} ParseTypeOrExprResult;

typedef enum {
	PARSE_TYPE_PARSED,
	PARSE_TYPE_NOT_PARSED,
	PARSE_TYPE_ERROR,
} ParseTypeResult;

ParseTypeResult _parser_try_parse_primitive_type(Parser* parser, ParsedType* out_type) {
	assert(out_type != NULL);

	Token token = preprocessor_view_next(parser->preprocessor);

	if (str_equal(token.string, STR_LIT("void"))) {
		preprocessor_next_token(parser->preprocessor);

		out_type->kind = PARSED_TYPE_VOID;
		return PARSE_TYPE_PARSED;
	} else if (str_equal(token.string, STR_LIT("float"))) {
		preprocessor_next_token(parser->preprocessor);

		out_type->kind = PARSED_TYPE_FLOAT;
		return PARSE_TYPE_PARSED;
	} else if (str_equal(token.string, STR_LIT("double"))) {
		preprocessor_next_token(parser->preprocessor);

		out_type->kind = PARSED_TYPE_DOUBLE;
		return PARSE_TYPE_PARSED;
	} else if (str_equal(token.string, STR_LIT("size_t"))) {
		preprocessor_next_token(parser->preprocessor);

		out_type->kind = PARSED_TYPE_SIZE_T;
		return PARSE_TYPE_PARSED;
	} else {
		ParsedTypeKindFlags type_flags = TYPE_FLAG_NONE;
		if (str_equal(token.string, STR_LIT("signed"))) {
			preprocessor_next_token(parser->preprocessor);

			type_flags |= TYPE_FLAG_SIGNED;
			token = preprocessor_view_next(parser->preprocessor);
		} else if (str_equal(token.string, STR_LIT("unsigned"))) {
			preprocessor_next_token(parser->preprocessor);

			type_flags |= TYPE_FLAG_UNSIGNED;
			token = preprocessor_view_next(parser->preprocessor);
		}

		ParsedTypeKind type_kind = INT32_MAX;

		if (str_equal(token.string, STR_LIT("char"))) {
			preprocessor_next_token(parser->preprocessor);
			type_kind = PARSED_TYPE_CHAR;
		} else if (str_equal(token.string, STR_LIT("int"))) {
			preprocessor_next_token(parser->preprocessor);
			type_kind = PARSED_TYPE_INT;
		} else if (str_equal(token.string, STR_LIT("short"))) {
			preprocessor_next_token(parser->preprocessor);
			type_kind = PARSED_TYPE_SHORT;
		} else if (str_equal(token.string, STR_LIT("long"))) {
			preprocessor_next_token(parser->preprocessor);

			Token next_token = preprocessor_view_next(parser->preprocessor);
			if (next_token.kind == TOKEN_IDENT && str_equal(next_token.string, STR_LIT("long"))) {
				preprocessor_next_token(parser->preprocessor);
				type_kind = PARSED_TYPE_LONG_LONG;
			} else {
				type_kind = PARSED_TYPE_LONG;
			}
		}

		if (type_kind != INT32_MAX) {
			out_type->kind = type_kind | type_flags;
			return PARSE_TYPE_PARSED;
		}

		if (type_kind == INT32_MAX && type_flags != TYPE_FLAG_NONE) {
			TokenKind expected_tokens[] = { TOKEN_IDENT };
			diagnostics_report_unexpected_token(parser->diagnostics, token, expected_tokens, array_size(expected_tokens));
			return PARSE_TYPE_ERROR;
		}
	}

	return PARSE_TYPE_NOT_PARSED;
}

ParseTypeResult _parser_try_parse_type_specifier(Parser* parser, ParsedType* out_type) {
	assert(out_type != NULL);

	Token token = preprocessor_view_next(parser->preprocessor);
	if (token.kind == TOKEN_IDENT) {
		if (is_digit(token.string.v[0])) {
			return PARSE_TYPE_NOT_PARSED;
		}

		ParseTypeResult primitive_parse_result = _parser_try_parse_primitive_type(parser, out_type);
		switch (primitive_parse_result) {
		case PARSE_TYPE_ERROR:
		case PARSE_TYPE_PARSED:
			return primitive_parse_result;
		case PARSE_TYPE_NOT_PARSED:
			break;
		}

		IdentifierEntry* entry = ident_storage_find(&parser->ident_storage, IDENT_NAMESPACE_ALIAS, token.string);
		if (entry == NULL) {
			return PARSE_TYPE_NOT_PARSED;

			// TODO: Generate error? In same cases it is needed
			diagnostics_report_error(parser->diagnostics,
					token.source_range,
					STR_LIT("Use of undeclared identifier"),
					NULL);
			return PARSE_TYPE_ERROR;
		}

		switch (entry->kind) {
		case IDENT_FUNCTION:
		case IDENT_VARIABLE:
			unreachable();
		case IDENT_TYPE_DEF:
			preprocessor_next_token(parser->preprocessor);

			*out_type = entry->type_def->aliased_type;
			out_type->alias_definition = entry->type_def;
			return PARSE_TYPE_PARSED;
		case IDENT_STRUCT:
			unreachable();
			preprocessor_next_token(parser->preprocessor);

			out_type->kind = PARSED_TYPE_STRUCT;
			out_type->struct_def = entry->struct_def;
			return PARSE_TYPE_PARSED;
		case IDENT_ENUM:
			unreachable();
			preprocessor_next_token(parser->preprocessor);

			out_type->kind = PARSED_TYPE_ENUM;
			out_type->enum_def = entry->enum_def;
			return PARSE_TYPE_PARSED;
		}

		unreachable();
	} else if (token.kind == TOKEN_KEYWORD_STRUCT) {
		ParsedStruct* struct_def = {};
		if (!_parser_parse_struct_def(parser, &struct_def)) {
			return PARSE_TYPE_ERROR;
		}

		out_type->kind = PARSED_TYPE_STRUCT;
		out_type->struct_def = struct_def;
		return PARSE_TYPE_PARSED;
	} else if (token.kind == TOKEN_KEYWORD_ENUM) {
		ParsedEnum* enum_def = {};
		if (!_parser_parse_enum_def(parser, &enum_def)) {
			return PARSE_TYPE_ERROR;
		}

		out_type->kind = PARSED_TYPE_ENUM;
		out_type->enum_def = enum_def;
		return PARSE_TYPE_PARSED;
	} else {
		preprocessor_next_token(parser->preprocessor);

		TokenKind expected_tokens[] = {
			TOKEN_IDENT,
			TOKEN_KEYWORD_CONST,
			TOKEN_KEYWORD_STRUCT,
			TOKEN_KEYWORD_ENUM,
		};

		diagnostics_report_unexpected_token(parser->diagnostics,
				token,
				expected_tokens,
				array_size(expected_tokens));
		return PARSE_TYPE_ERROR;
	}

	unreachable();
	return PARSE_TYPE_ERROR;
}

bool _parser_parse_type(Parser* parser, ParsedType* out_type) {
	assert(out_type != NULL);

	// First parse qualifiers
	out_type->qualifiers = _parser_parse_type_qualifiers(parser);

	switch (_parser_try_parse_type_specifier(parser, out_type)) {
	case PARSE_TYPE_ERROR:
	case PARSE_TYPE_NOT_PARSED:
		return false;
	case PARSE_TYPE_PARSED:
		return true;
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

	if (!_parser_parse_pre_declaration_modifiers(parser, &aliased_type, &aliased_type, true)) {
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

	ParsedTypeDef* type_def = arena_alloc(parser->ast_allocator, ParsedTypeDef);
	memset(type_def, 0, sizeof(*type_def));
	
	type_def->new_name = source_string_from_token(new_name);
	type_def->aliased_type = aliased_type;

	IdentifierEntry* entry = ident_storage_find(&parser->ident_storage, IDENT_NAMESPACE_ALIAS, new_name.string);
	if (!entry) {
		entry = ident_storage_insert(&parser->ident_storage, IDENT_NAMESPACE_ALIAS, source_string_from_token(new_name));
		entry->kind = IDENT_TYPE_DEF;
	}

	entry->type_def = type_def;

	ParsedNode* node = arena_alloc(parser->ast_allocator, ParsedNode);
	node->kind = AST_NODE_TYPE_DEF;
	node->type_def = type_def;
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

		if (!_parser_parse_pre_declaration_modifiers(parser, &param->type, &param->type, true)) {
			arena_end_temp(temp);
			return false;
		}

		Token token = preprocessor_view_next(parser->preprocessor);
		if (token.kind == TOKEN_IDENT) {
			preprocessor_next_token(parser->preprocessor); // consume param name

			param->name = source_string_from_token(token);
			token = preprocessor_view_next(parser->preprocessor);
		}

		if (!_parser_parse_post_declaration_modifiers(parser, &param->type, &param->type, true)) {
			arena_end_temp(temp);
			return false;
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

bool _token_kind_to_bin_op(TokenKind kind, BinOpKind* out_op) {
	switch (kind) {
	case TOKEN_PLUS:
		*out_op = BIN_OP_ADD;
		return true;
	case TOKEN_MINUS:
		*out_op = BIN_OP_SUB;
		return true;
	case TOKEN_ASTERISK:
		*out_op = BIN_OP_MUL;
		return true;
	case TOKEN_FORWARD_SLASH:
		*out_op = BIN_OP_DIV;
		return true;
	case TOKEN_PERCENT:
		*out_op = BIN_OP_MOD;
		return true;
	default:
		return false;
	}

	return false;
}

bool _token_kind_to_unary_pre_op(TokenKind kind, UnaryOpKind* out_op) {
#define ret(op) *out_op = op; return true;
	switch (kind) {
	case TOKEN_PLUS: ret(UNARY_OP_PLUS);
	case TOKEN_MINUS: ret(UNARY_OP_NEGATE);
	case TOKEN_DOUBLE_PLUS: ret(UNARY_OP_PRE_INCREMENT);
	case TOKEN_DOUBLE_MINUS: ret(UNARY_OP_PRE_DECREMENT);
	case TOKEN_AMPERSAND: ret(UNARY_OP_ADDRESS);
	case TOKEN_ASTERISK: ret(UNARY_OP_DEREFERENCE);
	case TOKEN_EXCLAMATION_MARK: ret(UNARY_OP_LOGICAL_NOT);
	case TOKEN_BITWISE_NOT: ret(UNARY_OP_BITWISE_NOT);
	default:
		return false;
	}
#undef ret

	unreachable();
	return false;
}

typedef enum {
	EXPR_PARSE_OK,
	EXPR_PARSE_NOT_PARSED,
	EXPR_PARSE_ERROR,
} ExprParseResult;

ExprParseResult _parser_try_parse_bin_expr_operand(Parser* parser, ParsedExpr* out_expr) {
	Token token = preprocessor_view_next(parser->preprocessor);

	UnaryOpKind unary_op;
	if (_token_kind_to_unary_pre_op(token.kind, &unary_op)) {
		preprocessor_next_token(parser->preprocessor);

		out_expr->kind = EXPR_UNARY;
		out_expr->unary.operand = arena_alloc(parser->ast_allocator, ParsedExpr);
		out_expr->unary.op = unary_op;

		return _parser_try_parse_bin_expr_operand(parser, out_expr->unary.operand);
	}
	
	if (token.kind == TOKEN_IDENT) {
		preprocessor_next_token(parser->preprocessor);

		assert(token.string.length > 0);
		bool is_int_literal = is_digit(token.string.v[0]);
		if (is_int_literal) {
			IntegerLiteralInfo literal_info = int_literal_info_from_token(token);

			uint64_t literal_value = 0;
			bool result = parse_integer_literal_value(parser->diagnostics,
					token,
					literal_info.int_part_string,
					literal_info.format,
					&literal_value);

			if (!result) {
				return EXPR_PARSE_ERROR;
			}

			assert(literal_value <= INT32_MAX);

			out_expr->kind = EXPR_INTEGER_LITERAL;
			out_expr->int_literal = (ParsedIntegerLiteral) {
				.source_range = token.source_range,
				.format = literal_info.format,
				.integer_type = PARSED_TYPE_INT,
				.value = literal_value,
			};

			return EXPR_PARSE_OK;
		}

		IdentifierEntry* entry = ident_storage_find(&parser->ident_storage, IDENT_NAMESPACE_DEFAULT, token.string);
		if (entry == NULL) {
			diagnostics_report_error(parser->diagnostics,
					token.source_range,
					STR_LIT("Use of undeclared identifier"),
					NULL);
			return EXPR_PARSE_ERROR;
		}

		switch (entry->kind) {
		case IDENT_FUNCTION:
			out_expr->kind = EXPR_FUNCTION_REFERENCE;
			out_expr->function_ref = entry->function_def;
			return EXPR_PARSE_OK;
		case IDENT_VARIABLE:
			out_expr->kind = EXPR_VARIABLE_REFERENCE;
			out_expr->variable_ref = entry->variable;
			return EXPR_PARSE_OK;
		case IDENT_TYPE_DEF:
		case IDENT_STRUCT:
		case IDENT_ENUM:
			unreachable();
		}

		unreachable();
	}

	return EXPR_PARSE_NOT_PARSED;
}

ExprParseResult _parser_try_parse_expr(Parser* parser, ParsedExpr* out_expr) {
	ExprParseResult left_operand_result = _parser_try_parse_bin_expr_operand(parser, out_expr);
	if (left_operand_result != EXPR_PARSE_OK) {
		return left_operand_result;
	}

	ParsedExpr* current_expr = out_expr;

	while (true) {
		Token op_token = preprocessor_view_next(parser->preprocessor);

		BinOpKind current_bin_op;
		if (_token_kind_to_bin_op(op_token.kind, &current_bin_op)) {
			preprocessor_next_token(parser->preprocessor);
				
			ParsedExpr* right_operand = arena_alloc(parser->ast_allocator, ParsedExpr);

			Token first_operand_token = preprocessor_view_next(parser->preprocessor);
			if (_parser_try_parse_bin_expr_operand(parser, right_operand) != EXPR_PARSE_OK) {
				diagnostics_report_error(parser->diagnostics,
						first_operand_token.source_range,
						STR_LIT("Expected binary expression operand"),
						NULL);
				return EXPR_PARSE_ERROR;
			}

			ParsedExpr* left_operand = arena_alloc(parser->ast_allocator, ParsedExpr);

			uint32_t current_op_precedence = bin_op_precedence(current_bin_op);
			uint32_t next_op_precedence = UINT32_MAX;

			{
				Token maybe_next_bin_op = preprocessor_view_next(parser->preprocessor);
				BinOpKind next_bin_op;
				if (_token_kind_to_bin_op(maybe_next_bin_op.kind, &next_bin_op)) {
					next_op_precedence = bin_op_precedence(next_bin_op);
				}
			}

			*left_operand = *current_expr;

			*current_expr = (ParsedExpr) {
				.kind = EXPR_BINARY,
				.binary = (ParsedBinExpr) {
					.op = current_bin_op,
					.left = left_operand,
					.right = right_operand,
				}
			};

			if (current_op_precedence > next_op_precedence) {
				current_expr = right_operand;
			}
		} else {
			break;
		}
	}

	return EXPR_PARSE_OK;
}

bool _parser_parse_pre_declaration_modifiers(Parser* parser,
		ParsedType* base_type,
		ParsedType* out_type,
		bool duplicate_base_type) {
	if (base_type == out_type) {
		assert(duplicate_base_type);
	}

	while (true) {
		TypeQualifiers qualifiers = _parser_parse_type_qualifiers(parser);

		Token maybe_asterisk = preprocessor_view_next(parser->preprocessor);
		if (maybe_asterisk.kind == TOKEN_ASTERISK) {
			preprocessor_next_token(parser->preprocessor);

			assert(duplicate_base_type);

			ParsedType* inner_type = arena_alloc(parser->ast_allocator, ParsedType);
			*inner_type = *base_type;

			*out_type = (ParsedType) {
				.kind = PARSED_TYPE_POINTER,
				.pointer_base_type = inner_type,
				.qualifiers = qualifiers,
			};
		} else {
			if (qualifiers != TYPE_QUALIFIER_NONE) {
				preprocessor_next_token(parser->preprocessor);

				TokenKind expected_tokens[] = { TOKEN_ASTERISK };
				diagnostics_report_unexpected_token(parser->diagnostics,
						maybe_asterisk,
						expected_tokens,
						array_size(expected_tokens));
				return false;
			} else {
				break;
			}
		}
	}

	return true;
}

bool _parser_parse_post_declaration_modifiers(Parser* parser,
		ParsedType* base_type,
		ParsedType* out_type,
		bool duplicate_base_type) {
	if (base_type == out_type) {
		assert(duplicate_base_type);
	}

	while (true) {
		Token token = preprocessor_view_next(parser->preprocessor);
		if (token.kind == TOKEN_LEFT_BRACKET) {
			assert(duplicate_base_type);

			preprocessor_next_token(parser->preprocessor);

			Token next_token = preprocessor_view_next(parser->preprocessor);
			bool has_size_expr = next_token.kind != TOKEN_RIGHT_BRACKET;

			ParsedExpr* size_expr = NULL;
			if (has_size_expr) {
				size_expr = arena_alloc(parser->ast_allocator, ParsedExpr);

				if (_parser_try_parse_expr(parser, size_expr) != EXPR_PARSE_OK) {
					diagnostics_report_error(parser->diagnostics,
							next_token.source_range,
							STR_LIT("Expected array size experession"),
							NULL);
					return false;
				}
			}

			Token closing_bracket = preprocessor_next_token(parser->preprocessor);
			if (closing_bracket.kind != TOKEN_RIGHT_BRACKET) {
				TokenKind expected_tokens[] = { TOKEN_RIGHT_BRACKET };
				diagnostics_report_unexpected_token(parser->diagnostics,
						closing_bracket,
						expected_tokens,
						array_size(expected_tokens));
				return false;
			}

			ParsedType* inner_type = arena_alloc(parser->ast_allocator, ParsedType);
			*inner_type = *base_type;

			*out_type = (ParsedType) {
				.kind = PARSED_TYPE_ARRAY,
				.array = {
					.element_type = inner_type,
					.size = size_expr,
				}
			};
		} else {
			break;
		}
	}

	return true;
}

bool _parser_parse_type_declaration(Parser* parser, ParsedNode* out_node, ParsedType* type) {
	assert(out_node != NULL);
	assert(type != NULL);

	if (!_parser_parse_pre_declaration_modifiers(parser, type, type, true)) {
		return false;
	}

	FunctionCallingConvention call_conv = FUNC_CALL_CONV_DEFAULT;

	Token name_token = preprocessor_next_token(parser->preprocessor);
	if (name_token.kind != TOKEN_IDENT) {
		TokenKind expected_tokens[] = { TOKEN_IDENT };
		diagnostics_report_unexpected_token(parser->diagnostics,
				name_token,
				expected_tokens,
				array_size(expected_tokens));
		
		return false;
	}

	if (str_equal(name_token.string, STR_LIT("__cdecl"))) {
		call_conv = FUNC_CALL_CONV_CDECL;

		// TODO: Clean this up a bit
		name_token = preprocessor_next_token(parser->preprocessor);
		if (name_token.kind != TOKEN_IDENT) {
			TokenKind expected_tokens[] = { TOKEN_IDENT };
			diagnostics_report_unexpected_token(parser->diagnostics,
					name_token,
					expected_tokens,
					array_size(expected_tokens));
			
			return false;
		}
	}

	SourceString name = source_string_from_token(name_token);

	bool is_function = false;
	ParsedFunctionParam* param_list = NULL;
	size_t param_count = 0;

	Token token = preprocessor_view_next(parser->preprocessor);
	if (token.kind == TOKEN_LEFT_PAREN) {
		if (!_parser_parse_function_param_list(parser, &param_list, &param_count)) {
			return false;
		}

		is_function = true;
	} else if (token.kind == TOKEN_EQUAL) {
		preprocessor_next_token(parser->preprocessor);

		ParsedExpr* value = arena_alloc(parser->ast_allocator, ParsedExpr);
		switch (_parser_try_parse_expr(parser, value)) {
		case EXPR_PARSE_OK:
			break;
		case EXPR_PARSE_ERROR:
			return false;
		case EXPR_PARSE_NOT_PARSED:
			diagnostics_report_error(parser->diagnostics,
					token.source_range,
					STR_LIT("Expected variable value after the '='"),
					NULL);
			return false;
		}

		out_node->kind = AST_NODE_VARIABLE;
		out_node->variable.name = name;
		out_node->variable.type = *type;
		out_node->variable.value = value;

		IdentifierEntry* entry = ident_storage_insert(&parser->ident_storage, IDENT_NAMESPACE_DEFAULT, out_node->variable.name);
		entry->kind = IDENT_VARIABLE;
		entry->variable = &out_node->variable;

		if (!_parser_expect_semicolon(parser, STR_LIT("Expected semicolon after the variable definition"))) {
			return false;
		}

		return true;
	} else if (token.kind == TOKEN_SEMICOLON) {
		preprocessor_next_token(parser->preprocessor);

	 	IdentifierEntry* existing_identifier = ident_storage_find(&parser->ident_storage,
				IDENT_NAMESPACE_DEFAULT,
				name.string);

		if (existing_identifier != NULL) {
			StringBuilder builder = { .arena = parser->diagnostics->allocator };
			str_builder_append(&builder, STR_LIT("Redefinition of '"));
			str_builder_append(&builder, name.string);
			str_builder_append_char(&builder, '\'');

			DiagnosticsEntry* error = diagnostics_report_error(parser->diagnostics,
					source_string_to_range(name),
					builder.string,
					NULL);

			diagnostics_report_error(parser->diagnostics,
					source_string_to_range(existing_identifier->name),
					STR_LIT("Previously defined here"),
					error);
			return false;
		} 

		// A variable declaration
		out_node->kind = AST_NODE_VARIABLE;
		out_node->variable = (ParsedVariable) {
			.name = name,
			.type = *type,
			.value = NULL,
		};

		IdentifierEntry* entry = ident_storage_insert(&parser->ident_storage,
				IDENT_NAMESPACE_DEFAULT,
				out_node->variable.name);

		entry->kind = IDENT_VARIABLE;
		entry->variable = &out_node->variable;

		return true;
	}

	if (is_function) {
		bool is_forward_declared = true;

		ParsedScope* body = {};
		Token token = preprocessor_view_next(parser->preprocessor);
		if (token.kind == TOKEN_LEFT_BRACE) {
			is_forward_declared = false;

			body = arena_alloc(parser->ast_allocator, ParsedScope);
			memset(body, 0, sizeof(*body));

			if (!_parser_parse_scope(parser, body)) {
				return false;
			}
		} else if (token.kind == TOKEN_SEMICOLON) {
			preprocessor_next_token(parser->preprocessor);
		} else {
			TokenKind expected_tokens[] = { TOKEN_LEFT_BRACE, TOKEN_SEMICOLON };
			diagnostics_report_unexpected_token(parser->diagnostics,
					token,
					expected_tokens,
					array_size(expected_tokens));
			return false;
		}

		IdentifierEntry* entry = ident_storage_find(&parser->ident_storage, IDENT_NAMESPACE_DEFAULT, name.string);
		ParsedFunction* function_def = NULL;
		if (entry) {
			if (!has_flag(entry->kind, IDENT_FUNCTION)) {
				StringBuilder builder = { .arena = parser->diagnostics->allocator };
				str_builder_append_char(&builder, '\'');
				str_builder_append(&builder, entry->name.string);
				str_builder_append(&builder, STR_LIT("' is previously defined with a different tag type"));

				DiagnosticsEntry* error = diagnostics_report_error(parser->diagnostics,
						source_string_to_range(name),
						builder.string,
						NULL);

				diagnostics_report_error(parser->diagnostics,
						source_string_to_range(entry->name),
						STR_LIT("Previously defined here"),
						error);
				return false;
			}

			function_def = entry->function_def;
			assert(function_def);

			// TODO: Verify that return types also match
			if (!function_def->is_forward_declared && !is_forward_declared) {
				StringBuilder builder = { .arena = parser->diagnostics->allocator };
				str_builder_append(&builder, STR_LIT("Redefinition of '"));
				str_builder_append(&builder, entry->name.string);
				str_builder_append_char(&builder, '\'');

				DiagnosticsEntry* error = diagnostics_report_error(parser->diagnostics,
						source_string_to_range(name),
						builder.string,
						NULL);

				diagnostics_report_error(parser->diagnostics,
						source_string_to_range(entry->name),
						STR_LIT("Previously defined here"),
						error);
				return false;
			}

			if (function_def->parameter_count != param_count) {
				DiagnosticsEntry* error = diagnostics_report_error(parser->diagnostics,
						source_string_to_range(name),
						STR_LIT("Function was previously defined with a different parameter count"),
						NULL);

				diagnostics_report_error(parser->diagnostics,
						source_string_to_range(entry->name),
						STR_LIT("Previously defined here"),
						error);
				return false;
			} else {
				ParsedFunctionParam* prev_def_param = function_def->parameter_list;
				ParsedFunctionParam* new_def_param = param_list;

				for (size_t i = 0; i < param_count; i += 1) {
					bool param_types_are_equal = type_equal(&prev_def_param->type, &new_def_param->type);

					if (!param_types_are_equal) {
						DiagnosticsEntry* error = diagnostics_report_error(parser->diagnostics,
							new_def_param->type.source_range,
							STR_LIT("Function previously defined with different parameter types"),
							NULL);

						diagnostics_report_error(parser->diagnostics,
								prev_def_param->type.source_range,
								STR_LIT("Previously defined here"),
								error);
						return false;
					}
					
					prev_def_param = prev_def_param->next;
					new_def_param = new_def_param->next;
				}
			}
		} else {
			entry = ident_storage_insert(&parser->ident_storage, IDENT_NAMESPACE_DEFAULT, name);
			entry->kind = IDENT_FUNCTION;

			function_def = arena_alloc(parser->ast_allocator, ParsedFunction); 
			memset(function_def, 0, sizeof(*function_def));
			
			function_def->name = name;
			function_def->return_type = *type;
			function_def->parameter_list = param_list;
			function_def->parameter_count = param_count;
			function_def->is_forward_declared = is_forward_declared;

			entry->function_def = function_def;
		}

		assert(function_def);

		if (is_forward_declared) {
			assert(body == NULL);
		} else {
			function_def->body = body;
			function_def->is_forward_declared = false;
			function_def->calling_convention = call_conv;
		}

		out_node->kind = AST_NODE_FUNCTION;
		out_node->function_def = function_def;
		return true;
	} else {
		TokenKind expected_tokens[] = { TOKEN_LEFT_PAREN };
		diagnostics_report_unexpected_token(parser->diagnostics,
				token,
				expected_tokens,
				array_size(expected_tokens));
		return false;
	}

	return true;
}

bool _parser_parse_variable_or_function_def(Parser* parser, ParsedNode* out_node) {
	assert(out_node != NULL);

	bool has_type = false;
	TypeQualifiers type_qualifiers = _parser_parse_type_qualifiers(parser);
	ParsedType type = { .qualifiers = type_qualifiers };

	Token maybe_type_specifier_token = preprocessor_view_next(parser->preprocessor);

	switch (_parser_try_parse_type_specifier(parser, &type)) {
	case PARSE_TYPE_PARSED:
		has_type = true;
		break;
	case PARSE_TYPE_NOT_PARSED:
		if (type_qualifiers != TYPE_QUALIFIER_NONE) {
			preprocessor_next_token(parser->preprocessor);
			diagnostics_report_error(parser->diagnostics,
					maybe_type_specifier_token.source_range,
					STR_LIT("Expected type specifier"),
					NULL);

			return false;
		}

		break;
	case PARSE_TYPE_ERROR:
		return false;
	}

	if (has_type) {
		return _parser_parse_type_declaration(parser, out_node, &type);
	} else {
		ExprParseResult result = _parser_try_parse_expr(parser, &out_node->expr);
		if (result == EXPR_PARSE_OK) {
			out_node->kind = AST_NODE_EXPR;

			if (!_parser_expect_semicolon(parser, STR_LIT("Expected ';' after an expression"))) {
				return false;
			}

			return true;
		} else {
			return false;
		}
	}

	unreachable();
	return false;
}

ParsedNode* _parser_parse_single_node(Parser* parser, Token initial_token) {
	switch (initial_token.kind) {
	case TOKEN_KEYWORD_TYPEDEF: {
		return _parser_parse_type_def(parser);
	}
	case TOKEN_KEYWORD_STRUCT: {
		ParsedStruct* struct_def = NULL;
		if (!_parser_parse_struct_def(parser, &struct_def)) {
			return NULL;
		}

		if (!_parser_expect_semicolon(parser, STR_LIT("Expected ';' after the struct"))) {
			return NULL;
		}

		ParsedNode* node = arena_alloc(parser->ast_allocator, ParsedNode);
		memset(node, 0, sizeof(*node));

		node->kind = AST_NODE_STRUCT;
		node->struct_def = struct_def;
		return node;
	}
	case TOKEN_KEYWORD_ENUM: {
		ParsedEnum* enum_def = NULL;

		if (!_parser_parse_enum_def(parser, &enum_def)) {
			return NULL;
		}

		if (!_parser_expect_semicolon(parser, STR_LIT("Expected ';' after the enum"))) {
			return NULL;
		}

		ParsedNode* node = arena_alloc(parser->ast_allocator, ParsedNode);
		memset(node, 0, sizeof(*node));

		node->kind = AST_NODE_ENUM;
		node->enum_def = enum_def;
		return node;
	}
	case TOKEN_KEYWORD_RETURN: {
		preprocessor_next_token(parser->preprocessor);

		Token token = preprocessor_view_next(parser->preprocessor);
		bool has_value = true;
		if (token.kind == TOKEN_SEMICOLON) {
			has_value = false;
		}
		
		ParsedExpr* return_value = NULL;
		if (has_value) {
			return_value = arena_alloc(parser->ast_allocator, ParsedExpr);

			if (_parser_try_parse_expr(parser, return_value) != EXPR_PARSE_OK) {
				return NULL;
			}
		}

		if (!_parser_expect_semicolon(parser, STR_LIT("Expected ';' after the return"))) {
			return NULL;
		}

		ParsedNode* node = arena_alloc(parser->ast_allocator, ParsedNode);
		memset(node, 0, sizeof(*node));

		node->kind = AST_NODE_RETURN;
		node->return_stmt.value = return_value;
		return node;
	}
	default: {
		ArenaRegion temp = arena_begin_temp(parser->ast_allocator);
		ParsedNode* node = arena_alloc(parser->ast_allocator, ParsedNode);
		memset(node, 0, sizeof(*node));

		if (_parser_parse_variable_or_function_def(parser, node)) {
			return node;
		} else {
			arena_end_temp(temp);
			_parser_skip_until_semicolon(parser);
			return NULL;
		}

		break;
	}
	}

	unreachable();
	return NULL;
}

bool _parser_parse_scope(Parser* parser, ParsedScope* out_scope) {
	ident_storage_begin_scope(&parser->ident_storage);

	Token token = preprocessor_next_token(parser->preprocessor);
	assert(token.kind == TOKEN_LEFT_BRACE);

	while (true) {
		Token token = preprocessor_view_next(parser->preprocessor);
		if (token.kind == TOKEN_EOF) {
			preprocessor_next_token(parser->preprocessor);

			diagnostics_report_error(parser->diagnostics,
					token.source_range,
					STR_LIT("Unexpected end of file"),
					NULL);

			ident_storage_end_scope(&parser->ident_storage);
			return false;
		} else if (token.kind == TOKEN_RIGHT_BRACE) {
			preprocessor_next_token(parser->preprocessor);
			break;
		} else if (token.kind == TOKEN_SEMICOLON) {
			preprocessor_next_token(parser->preprocessor);
			continue;
		}

		ParsedNode* node = _parser_parse_single_node(parser, token);
		if (node) {
			parsed_node_list_append(&out_scope->nodes, node);
		}
	}

	ident_storage_end_scope(&parser->ident_storage);

	return true;
}

void parser_init(Parser* parser,
		Arena* ast_allocator,
		Arena* ident_allocator,
		Preprocessor* preprocessor,
		Diagnostics* diagnostics) {
	parser->ast_allocator = ast_allocator;
	parser->diagnostics = diagnostics;
	parser->preprocessor = preprocessor;

	ident_storage_init(&parser->ident_storage, ident_allocator);
}

void parser_parse(Parser* parser, ParsedAST* ast) {
	ast->root_nodes = (ParsedNodeList) {};

	bool run = true;
	while (run) {
		Token token = preprocessor_view_next(parser->preprocessor);
		if (token.kind == TOKEN_EOF) {
			preprocessor_next_token(parser->preprocessor);
			break;
		} else if (token.kind == TOKEN_SEMICOLON) {
			preprocessor_next_token(parser->preprocessor);
			continue;
		}

		ParsedNode* node = _parser_parse_single_node(parser, token);
		if (node) {
			parsed_node_list_append(&ast->root_nodes, node);
		}
	}
}
