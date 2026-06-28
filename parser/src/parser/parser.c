#include "parser.h"

#include "core/profiler.h"

#include "parser/parse_tools.h"

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

inline void _ident_storage_alloc_namespace_hash_map(IdentifierNamespace* ident_namespace, Allocator allocator) {
	size_t key_size = sizeof(String);
	size_t entry_size = sizeof(IdentifierEntry*);
	size_t buffer_size = (key_size + entry_size) * ident_namespace->capacity;
	uint8_t* new_buffer = allocator_alloc_bytes(allocator, buffer_size, alignof(max_align_t));

	memset(new_buffer, 0, buffer_size);

	ident_namespace->keys = (String*)new_buffer;
	ident_namespace->entries = (IdentifierEntry**)(new_buffer + key_size * ident_namespace->capacity);
}

static void _ident_storage_grow_namespace(IdentifierNamespace* ident_namespace, Allocator allocator) {
	profile_scope_start("_ident_storage_grow_namespace");

	assert(ident_namespace);

	if (ident_namespace->count == 0) {
		assert(ident_namespace->keys == NULL);
		assert(ident_namespace->entries == NULL);
	}

	size_t old_capacity = ident_namespace->capacity;
	String* old_keys = ident_namespace->keys;
	IdentifierEntry** old_entries = ident_namespace->entries;

	ident_namespace->capacity = ident_namespace->capacity + ident_namespace->capacity / 2;
	_ident_storage_alloc_namespace_hash_map(ident_namespace, allocator);

	for (size_t i = 0; i < old_capacity; i += 1) {
		if (old_keys[i].v == NULL || old_keys[i].v == REMOVED_SLOT_FLAG) {
			continue;
		}

		size_t empty_slot = _ident_storage_try_find_empty_entry(ident_namespace, old_keys[i]);
		assert(empty_slot != SIZE_MAX);

		ident_namespace->keys[empty_slot] = old_keys[i];
		ident_namespace->entries[empty_slot] = old_entries[i];
	}

	profile_scope_end();
}

inline bool _ident_storage_try_insert(IdentifierNamespace* ident_namespace,
		Allocator allocator,
		String name,
		IdentifierEntry* entry) {

	if (ident_namespace->count + 1 == ident_namespace->capacity / 2) {
		_ident_storage_grow_namespace(ident_namespace, allocator);
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

IdentifierEntry* ident_storage_find(IdentifierStorage* storage,
		IdentifierNamespaceKind namespace_kind,
		IdentifierFindOption option,
		String name) {

	IdentifierNamespace* ident_namespace = _ident_storage_get_namespace(storage, namespace_kind);
	size_t index = _ident_storage_try_find_entry(ident_namespace, name);
	if (index == SIZE_MAX) {
		return NULL;
	}

	IdentifierEntry* entry = ident_namespace->entries[index];
	switch (option) {
	case IDENT_FIND_IN_CURRENT_SCOPE:
		if (entry->owner_scope == storage->current_scope) {
			return entry;
		}

		return NULL;
	case IDENT_FIND_IN_ALL_PARENT_SCOPES:
		return entry;
	}

	unreachable();
	return NULL;
}

void _ident_storage_init_namespace(IdentifierNamespace* ident_namespace) {
}

void ident_storage_init(IdentifierStorage* storage, Allocator namespace_allocator, Arena* allocator) {
	assert(storage != NULL);

	storage->allocator = allocator;
	storage->namespace_allocator = namespace_allocator;

	for (size_t i = 0; i < IDENT_NAMESPACE_COUNT; i += 1) {
		_ident_storage_init_namespace(&storage->namespaces[i]);

		storage->namespaces[i].count = 0;
		storage->namespaces[i].capacity = 64;

		_ident_storage_alloc_namespace_hash_map(&storage->namespaces[i], storage->namespace_allocator);
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

			allocator_release(storage->namespace_allocator, ident_namespace->keys);

			ident_namespace->keys = NULL;
			ident_namespace->entries = NULL;
		}
	}
	
}

static IdentifierEntry* _ident_storage_alloc_entry(IdentifierStorage* storage) {
	IdentifierEntry* entry = NULL;
	if (storage->next_free_entry) {
		// Reuse a free entry
		entry = storage->next_free_entry;
		storage->next_free_entry = storage->next_free_entry->next_in_scope;
	} else {
		entry = arena_alloc(storage->allocator, IdentifierEntry);
	}

	assert(entry);

	memset(entry, 0, sizeof(*entry));
	return entry;
}

IdentifierEntry* ident_storage_insert(IdentifierStorage* storage,
		IdentifierNamespaceKind namespace_kind,
		IdentifierEntryKind entry_kind,
		SourceString name) {

	assert(storage != NULL);
	assert(name.string.length > 0);
	assert(storage->current_scope);

	IdentifierNamespace* ident_namespace = _ident_storage_get_namespace(storage, namespace_kind);
	IdentifierEntry* entry = NULL;

	size_t existing_entry_index = _ident_storage_try_find_entry(ident_namespace, name.string);
	if (existing_entry_index == SIZE_MAX) {
		entry = _ident_storage_alloc_entry(storage);
		entry->kind = entry_kind;

		bool entry_inserted = _ident_storage_try_insert(ident_namespace, storage->namespace_allocator, name.string, entry);
		assert(entry_inserted);
	} else {
		assert_msg(ident_namespace->entries[existing_entry_index]->owner_scope != storage->current_scope,
				"Identifier with the given name is already defined in the current scope");

		entry = _ident_storage_alloc_entry(storage);
		entry->kind = entry_kind;

		entry->prev = ident_namespace->entries[existing_entry_index];
		ident_namespace->entries[existing_entry_index] = entry;
	}

	entry->name = name;
	entry->owner_namespace = namespace_kind;
	entry->owner_scope = storage->current_scope;

	{
		IdentifierScope* current_scope = storage->current_scope;

		// Add to the current scope
		if (current_scope->first_identifier == NULL) {
			current_scope->first_identifier = entry;
		} else {
			current_scope->last_identifier->next_in_scope = entry;
		}

		current_scope->last_identifier = entry;

		uint8_t entry_kind_index = ((uint8_t)entry_kind) & IDENT_ENTRY_INDEX_MASK;
		assert((size_t)entry_kind_index < IDENT_ENTRY_KIND_COUNT);

		current_scope->nested_entry_count[entry_kind_index] += 1;
		current_scope->entry_count[entry_kind_index] += 1;
	}

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
	storage->next_free_entry = entry;
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
	if (parent_scope) {
		for (size_t i = 0; i < IDENT_ENTRY_KIND_COUNT; i += 1) {
			parent_scope->nested_entry_count[i] += scope->nested_entry_count[i];

			assert(parent_scope->nested_entry_count[i] >= parent_scope->entry_count[i]);
		}
	}

	scope->next_free = storage->next_free_scope;
	storage->next_free_scope = scope;

	storage->current_scope = parent_scope;
}

//
// Parser
//

bool _parser_parse_type(Parser* parser, Type* out_type, bool is_anonymous);
bool _parser_parse_scope(Parser* parser, Scope* out_scope);
bool _parser_parse_pre_declaration_modifiers(Parser* parser,
		Type* base_type,
		Type* out_type,
		bool duplicate_base_type);

bool _parser_parse_post_declaration_modifiers(Parser* parser,
		Type* base_type,
		Type* out_type,
		bool duplicate_base_type);

static AstNode* _parser_parse_single_node(Parser* parser, Token initial_token);

typedef enum {
	EXPR_PARSE_OK,
	EXPR_PARSE_NOT_PARSED,
	EXPR_PARSE_ERROR,
} ExprParseResult;

static ExprParseResult _parser_try_parse_expr(Parser* parser, Expr* out_expr);
static ExprParseResult _parser_try_parse_bin_expr_operand(Parser* parser, Expr* out_expr);

//
// Parser Implementation
//

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

// TODO: Don't reset `ast_allocator` because, during testing that same `ast_allocator`
//       is used for diagnostics and reseting it corrupts diagnostics state
static bool _parser_parse_struct_fields(Parser* parser, size_t* out_field_count, StructField** out_fields) {
	assert(out_field_count != NULL);
	assert(out_fields != NULL);

	Token left_brace = preprocessor_next_token(parser->preprocessor);
	assert(left_brace.kind == TOKEN_LEFT_BRACE);

	ArenaRegion ast_temp = arena_begin_temp(parser->ast_allocator);
	ArenaRegion temp = arena_begin_temp(parser->temp_allocator);

	StructField* fields = arena_alloc_array(parser->temp_allocator, StructField, 0);
	size_t field_count = 0;

	while (true) {
		{
			Token token = preprocessor_view_next(parser->preprocessor);
			if (token.kind == TOKEN_RIGHT_BRACE) {
				preprocessor_next_token(parser->preprocessor);
				break;
			}
		}

		StructField* field = arena_alloc_zeroed(parser->temp_allocator, StructField);
		field_count += 1;

		if (!_parser_parse_type(parser, &field->type, true)) {
			arena_end_temp(ast_temp);
			arena_end_temp(temp);
			return false;
		}

		if (!_parser_parse_pre_declaration_modifiers(parser, &field->type, &field->type, true)) {
			arena_end_temp(ast_temp);
			arena_end_temp(temp);
			return false;
		}

		Token name_or_semilcolon = preprocessor_view_next(parser->preprocessor);
		if (name_or_semilcolon.kind == TOKEN_IDENT) {
			field->name = source_string_from_token(name_or_semilcolon);
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
	}

	*out_field_count = field_count;
	if (field_count == 0) {
		*out_fields = NULL;
	} else {
		*out_fields = arena_alloc_array(parser->ast_allocator, StructField, field_count);
		array_copy(*out_fields, fields, field_count);
	}

	arena_end_temp(temp);
	return true;
}

typedef struct {
	const Struct* struct_def;
	size_t field_index;
} NamedFieldLocation;

typedef struct {
	NamedFieldLocation* locations;
	size_t count;
	Arena* allocator;
} NamedFieldLocationArray;

static void _parser_gather_named_field_locations_of_anonymous_type_defs(const Struct* struct_def,
		NamedFieldLocationArray* out_field_locations) {

	for (size_t i = 0; i < struct_def->field_count; i += 1) {
		const StructField* field = &struct_def->fields[i];
		if (field->name.string.length == 0) {
			if (field->type.kind == TYPE_STRUCT) {
				const Struct* inner_def = field->type.struct_def;
				_parser_gather_named_field_locations_of_anonymous_type_defs(inner_def, out_field_locations);
			} else if (field->type.kind == TYPE_UNION) {
				const Struct* inner_def = field->type.union_def;
				_parser_gather_named_field_locations_of_anonymous_type_defs(inner_def, out_field_locations);
			}
		} else {
			arena_alloc(out_field_locations->allocator, NamedFieldLocation);
			out_field_locations->locations[out_field_locations->count] = (NamedFieldLocation) {
				.struct_def = struct_def,
				.field_index = i,
			};
			out_field_locations->count += 1;
		}
	}
}

static void _parser_initialize_struct_fields_namespace(Struct* struct_def,
		Arena* allocator,
		Arena* temp_allocator) {

	profile_scope_start(__func__);

	ArenaRegion temp = arena_begin_temp(temp_allocator);
	NamedFieldLocationArray named_field_locations = {};
	named_field_locations.locations = arena_alloc_array(temp_allocator, NamedFieldLocation, 0);
	named_field_locations.count = 0;
	named_field_locations.allocator = temp_allocator;

	_parser_gather_named_field_locations_of_anonymous_type_defs(
			struct_def,
			&named_field_locations);

	StructFieldNamespace* field_namespace = arena_alloc_zeroed(allocator, StructFieldNamespace);
	field_namespace->size = 0;
	field_namespace->capacity = named_field_locations.count * 2;
	field_namespace->keys = arena_alloc_array_zeroed(allocator,
			String,
			field_namespace->capacity);

	field_namespace->entries = arena_alloc_array_zeroed(allocator,
			StructFieldNamespaceEntry,
			field_namespace->capacity);

	for (size_t i = 0; i < named_field_locations.count; i += 1) {
		NamedFieldLocation loc = named_field_locations.locations[i];
		const StructField* field = &loc.struct_def->fields[loc.field_index];
		size_t index = hash_string(field->name.string) % field_namespace->capacity;

		while (true) {
			String key = field_namespace->keys[index];
			if (key.v == NULL) {
				field_namespace->keys[index] = field->name.string;
				field_namespace->entries[index] = (StructFieldNamespaceEntry) {
					.struct_def = loc.struct_def,
					.field_index = loc.field_index,
				};

				field_namespace->size += 1;
				break;
			} else if (str_equal(key, field->name.string)) {
				panic("Duplicate struct fields. TODO: Handle");
			}

			index = (index + 1) % field_namespace->capacity;
		}
	}

	struct_def->field_namespace = field_namespace;
	
	arena_end_temp(temp);

	profile_scope_end();
}

bool _parser_parse_struct_def(Parser* parser, Struct** out_struct_def, bool is_anonymous) {
	assert(out_struct_def != NULL);

	Token keyword_token = preprocessor_next_token(parser->preprocessor);
	assert(keyword_token.kind == TOKEN_KEYWORD_STRUCT || keyword_token.kind == TOKEN_KEYWORD_UNION);

	bool is_struct = keyword_token.kind == TOKEN_KEYWORD_STRUCT;
	StructLayoutKind layout_kind = is_struct
		? STRUCT_LAYOUT_KIND_STRUCT
		: STRUCT_LAYOUT_KIND_UNION;
	IdentifierEntryKind ident_kind = is_struct
		? IDENT_STRUCT
		: IDENT_UNION;

	SourceString struct_name = {};
	StructField* fields = NULL;
	size_t field_count = 0;
	bool is_forward_declared = true;

	Token token = preprocessor_view_next(parser->preprocessor);
	if (token.kind == TOKEN_IDENT) {
		preprocessor_next_token(parser->preprocessor); // consume identifier

		struct_name = source_string_from_token(token);
		token = preprocessor_view_next(parser->preprocessor);
	}

	if (token.kind == TOKEN_LEFT_BRACE) {
		is_forward_declared = false;
		if (!_parser_parse_struct_fields(parser, &field_count, &fields)) {
			return false;
		}
	}

	bool struct_def_initialized = false;
	Struct* struct_def = NULL;
	if (struct_name.string.length == 0) {
		struct_def = arena_alloc_zeroed(parser->ast_allocator, Struct);
		struct_def_initialized = false;
	} else if (!is_anonymous) {
		IdentifierEntry* entry = ident_storage_find(parser->ident_storage,
				IDENT_NAMESPACE_TAGGED,
				IDENT_FIND_DEFAULT,
				struct_name.string);
		if (entry) {
			if (!has_flag(entry->kind, ident_kind)) {
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
			
			struct_def = is_struct ? entry->struct_def : entry->union_def;
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
			entry = ident_storage_insert(parser->ident_storage, IDENT_NAMESPACE_TAGGED, ident_kind, struct_name);
			struct_def = arena_alloc_zeroed(parser->ast_allocator, Struct);

			if (is_struct) {
				entry->struct_def = struct_def;
			} else {
				entry->union_def = struct_def;
			}
		}
	} else {
		struct_def = arena_alloc_zeroed(parser->ast_allocator, Struct);
		struct_def_initialized = false;
	}

	assert(struct_def);

	if (!struct_def_initialized) {
		struct_def->name = struct_name;
		struct_def->layout_kind = layout_kind;
		struct_def->is_forward_declared = is_forward_declared;
	}

	if (is_forward_declared) {
		assert(field_count == 0);
		assert(fields == NULL);
	} else {
		struct_def->field_count = field_count;
		struct_def->fields = fields;
		struct_def->is_forward_declared = false;

		if (!is_anonymous) {
			_parser_initialize_struct_fields_namespace(struct_def, parser->ast_allocator, parser->temp_allocator);
		}
	}

	*out_struct_def = struct_def;
	return true;
}

bool _parser_parse_enum_variants(Parser* parser, size_t* out_variant_count, EnumVariant** out_variants) {
	assert(out_variant_count != NULL);
	assert(out_variants != NULL);

	{
		Token left_brace = preprocessor_next_token(parser->preprocessor);
		assert(left_brace.kind == TOKEN_LEFT_BRACE);
	}

	ArenaRegion temp = arena_begin_temp(parser->temp_allocator);

	bool result = true;
	size_t variant_count = 0;
	EnumVariant* variants = arena_alloc_array(parser->temp_allocator, EnumVariant, 0);

	while (true) {
		{
			Token token = preprocessor_view_next(parser->preprocessor);
			if (token.kind == TOKEN_RIGHT_BRACE) {
				preprocessor_next_token(parser->preprocessor);
				break;
			}
		}

		Token name_token = preprocessor_next_token(parser->preprocessor);
		if (name_token.kind != TOKEN_IDENT) {
			TokenKind expected_tokens[] = { TOKEN_IDENT };
			diagnostics_report_unexpected_token(parser->diagnostics,
					name_token,
					expected_tokens,
					array_size(expected_tokens));
			result = false;
			break;
		}

		EnumVariant* variant = arena_alloc_zeroed(parser->temp_allocator, EnumVariant);
		variant->name = source_string_from_token(name_token);
		variant_count += 1;

		// Parse an optional value
		Token equal_token = preprocessor_view_next(parser->preprocessor);
		if (equal_token.kind == TOKEN_EQUAL) {
			preprocessor_next_token(parser->preprocessor);

			Expr* value = arena_alloc(parser->ast_allocator, Expr);
			switch (_parser_try_parse_expr(parser, value)) {
			case EXPR_PARSE_OK:
				variant->value = value;
				break;
			case EXPR_PARSE_ERROR:
			case EXPR_PARSE_NOT_PARSED:
				diagnostics_report_error(parser->diagnostics,
						equal_token.source_range,
						STR_LIT("Expected a enum variant value"),
						NULL);
				break;
			}
		}

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
			result = false;
			break;
		}
	}

	if (!result) {
		arena_end_temp(temp);
		return false;
	}

	if (variant_count > 0) {
		*out_variant_count = variant_count;
		*out_variants = arena_alloc_array(parser->ast_allocator, EnumVariant, variant_count);
		array_copy(*out_variants, variants, variant_count);
	} else {
		*out_variant_count = 0;
		*out_variants = NULL;
	}

	arena_end_temp(temp);
	return true;
}

void _parser_register_enum_variants(Parser* parser, Enum* enum_def) {
	for (size_t i = 0; i < enum_def->variant_count; i += 1) {
		EnumVariant variant = enum_def->variants[i];
		IdentifierEntry* entry = ident_storage_find(parser->ident_storage,
				IDENT_NAMESPACE_DEFAULT,
				IDENT_FIND_DEFAULT,
				variant.name.string);

		if (entry) {
			StringBuilder builder = { .arena = parser->diagnostics->allocator };
			str_builder_append(&builder, STR_LIT("Name \'"));
			str_builder_append(&builder, entry->name.string);
			str_builder_append(&builder, STR_LIT("' is already defined"));

			DiagnosticsEntry* error = diagnostics_report_error(parser->diagnostics,
					source_string_to_range(variant.name),
					builder.string,
					NULL);

			diagnostics_report_error(parser->diagnostics,
					source_string_to_range(entry->name),
					STR_LIT("Previously defined here"),
					error);
		} else {
			entry = ident_storage_insert(parser->ident_storage,
					IDENT_NAMESPACE_DEFAULT,
					IDENT_ENUM_CONSTANT,
					variant.name);

			entry->enum_constant.enum_def = enum_def;
			entry->enum_constant.variant_index = i;
		}
	}
}

bool _parser_parse_enum_def(Parser* parser, Enum** out_enum_def, bool is_anonymous) {
	assert(out_enum_def != NULL);

	Token keyword_token = preprocessor_next_token(parser->preprocessor);
	assert(keyword_token.kind == TOKEN_KEYWORD_ENUM);

	SourceString enum_name = {};
	EnumVariant* variants = NULL;
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
		if (!_parser_parse_enum_variants(parser, &variant_count, &variants)) {
			return false;
		}
	}

	if (variants != NULL) {
		assert(variant_count > 0);
	}

	Enum* enum_def = NULL;
	if (enum_name.string.length > 0 && !is_anonymous) {
		IdentifierEntry* entry = ident_storage_find(parser->ident_storage,
				IDENT_NAMESPACE_TAGGED,
				IDENT_FIND_DEFAULT,
				enum_name.string);

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
			entry = ident_storage_insert(parser->ident_storage, IDENT_NAMESPACE_TAGGED, IDENT_ENUM, enum_name);

			enum_def = arena_alloc(parser->ast_allocator, Enum);
			memset(enum_def, 0, sizeof(*enum_def));
			
			enum_def->name = enum_name;
			enum_def->is_forward_declared = is_forward_declared;

			entry->enum_def = enum_def;
		}
	} else {
		enum_def = arena_alloc(parser->ast_allocator, Enum);
		memset(enum_def, 0, sizeof(*enum_def));

		enum_def->name = enum_name;
		enum_def->is_forward_declared = is_forward_declared;
	}

	assert(enum_def);

	if (is_forward_declared) {
		assert(variant_count == 0);
		assert(variants == NULL);
	} else {
		enum_def->variant_count = variant_count;
		enum_def->variants = variants;
		enum_def->is_forward_declared = false;

		_parser_register_enum_variants(parser, enum_def);
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
	TYPE_OR_EXPR_TYPE,
	TYPE_OR_EXPR_EXPR,
} ParseTypeOrExprResult;

typedef enum {
	PARSE_TYPE_PARSED,
	PARSE_TYPE_NOT_PARSED,
	PARSE_TYPE_ERROR,
} ParseTypeResult;

ParseTypeResult _parser_try_parse_primitive_type(Parser* parser, Type* out_type) {
	assert(out_type != NULL);

	Token token = preprocessor_view_next(parser->preprocessor);
	if (token.kind == TOKEN_KEYWORD_VOID) {
		preprocessor_next_token(parser->preprocessor);
		out_type->kind = TYPE_VOID;
		return PARSE_TYPE_PARSED;
	} else if (token.kind == TOKEN_KEYWORD_FLOAT) {
		preprocessor_next_token(parser->preprocessor);
		out_type->kind = TYPE_FLOAT;
		return PARSE_TYPE_PARSED;
	} else if (token.kind == TOKEN_KEYWORD_DOUBLE) {
		preprocessor_next_token(parser->preprocessor);
		out_type->kind = TYPE_DOUBLE;
		return PARSE_TYPE_PARSED;
	} else if (token.kind == TOKEN_KEYWORD_SIZE_T) {
		preprocessor_next_token(parser->preprocessor);
		out_type->kind = TYPE_SIZE_T;
		return PARSE_TYPE_PARSED;
	}

	TypeKind type_kind = TYPE_VOID;
	TypeKindFlags type_flags = TYPE_FLAG_NONE;
	if (token.kind == TOKEN_KEYWORD_SIGNED) {
		preprocessor_next_token(parser->preprocessor);

		type_kind = TYPE_INT;
		type_flags |= TYPE_FLAG_SIGNED;
		token = preprocessor_view_next(parser->preprocessor);
	} else if (token.kind == TOKEN_KEYWORD_UNSIGNED) {
		preprocessor_next_token(parser->preprocessor);

		type_kind = TYPE_INT;
		type_flags |= TYPE_FLAG_UNSIGNED;
		token = preprocessor_view_next(parser->preprocessor);
	}

	if (token.kind == TOKEN_KEYWORD_CHAR) {
		preprocessor_next_token(parser->preprocessor);
		type_kind = TYPE_CHAR;
	} else if (token.kind == TOKEN_KEYWORD_INT) {
		preprocessor_next_token(parser->preprocessor);
		type_kind = TYPE_INT;
	} else if (token.kind == TOKEN_KEYWORD_SHORT) {
		preprocessor_next_token(parser->preprocessor);
		type_kind = TYPE_SHORT;
	} else if (token.kind == TOKEN_KEYWORD_INT8) {
		preprocessor_next_token(parser->preprocessor);
		type_kind = TYPE_INT8;
	} else if (token.kind == TOKEN_KEYWORD_INT16) {
		preprocessor_next_token(parser->preprocessor);
		type_kind = TYPE_INT16;
	} else if (token.kind == TOKEN_KEYWORD_INT32) {
		preprocessor_next_token(parser->preprocessor);
		type_kind = TYPE_INT32;
	} else if (token.kind == TOKEN_KEYWORD_INT64) {
		preprocessor_next_token(parser->preprocessor);
		type_kind = TYPE_INT64;
	} else if (token.kind == TOKEN_KEYWORD_LONG) {
		preprocessor_next_token(parser->preprocessor);

		Token next_token = preprocessor_view_next(parser->preprocessor);
		if (next_token.kind == TOKEN_KEYWORD_LONG) {
			preprocessor_next_token(parser->preprocessor);
			type_kind = TYPE_LONG_LONG;
		} else {
			type_kind = TYPE_LONG;
		}
	}

	if (type_kind != TYPE_VOID) {
		out_type->kind = type_kind | type_flags;
		return PARSE_TYPE_PARSED;
	}

	return PARSE_TYPE_NOT_PARSED;
}

ParseTypeResult _parser_try_parse_type_specifier(Parser* parser, Type* out_type, bool is_anonymous) {
	assert(out_type != NULL);

	ParseTypeResult primitive_parse_result = _parser_try_parse_primitive_type(parser, out_type);
	switch (primitive_parse_result) {
	case PARSE_TYPE_ERROR:
	case PARSE_TYPE_PARSED:
		return primitive_parse_result;
	case PARSE_TYPE_NOT_PARSED:
		break;
	}

	Token token = preprocessor_view_next(parser->preprocessor);
	if (token.kind == TOKEN_IDENT) {
		if (is_digit(token.string.v[0])) {
			return PARSE_TYPE_NOT_PARSED;
		}

		IdentifierEntry* entry = ident_storage_find(parser->ident_storage,
				IDENT_NAMESPACE_ALIAS,
				IDENT_FIND_DEFAULT,
				token.string);
		if (entry == NULL) {
			return PARSE_TYPE_NOT_PARSED;
		}

		// NOTE: Here we since we only do search in the alias namespace,
		//       we only care about the type def, and since the alias namespace
		//       contains only type defs, any other ident kind is expected

		switch (entry->kind) {
		case IDENT_TYPE_DEF:
			preprocessor_next_token(parser->preprocessor);

			*out_type = entry->type_def->aliased_type;
			out_type->alias_definition = entry->type_def;
			return PARSE_TYPE_PARSED;
		case IDENT_FUNCTION:
		case IDENT_VARIABLE:
		case IDENT_STRUCT:
		case IDENT_UNION:
		case IDENT_ENUM:
		case IDENT_ENUM_CONSTANT:
		case IDENT_FUNCTION_PARAM:
		case IDENT_KIND_MAX:
			unreachable();
		}

		unreachable();
	} else if (token.kind == TOKEN_KEYWORD_STRUCT || token.kind == TOKEN_KEYWORD_UNION) {
		Struct* struct_def = {};
		if (!_parser_parse_struct_def(parser, &struct_def, is_anonymous)) {
			return PARSE_TYPE_ERROR;
		}


		if (struct_def->layout_kind == STRUCT_LAYOUT_KIND_STRUCT) {
			out_type->kind = TYPE_STRUCT;
			out_type->struct_def = struct_def;
		} else {
			out_type->kind = TYPE_UNION;
			out_type->union_def = struct_def;
		}

		return PARSE_TYPE_PARSED;
	} else if (token.kind == TOKEN_KEYWORD_ENUM) {
		Enum* enum_def = {};
		if (!_parser_parse_enum_def(parser, &enum_def, is_anonymous)) {
			return PARSE_TYPE_ERROR;
		}

		out_type->kind = TYPE_ENUM;
		out_type->enum_def = enum_def;
		return PARSE_TYPE_PARSED;
	}

	return PARSE_TYPE_NOT_PARSED;
}

static ParseTypeResult _parser_try_parse_type_name(Parser* parser, Type* out_type) {
	TypeQualifiers qualifiers = _parser_parse_type_qualifiers(parser);
	
	switch (_parser_try_parse_primitive_type(parser, out_type)) {
	case PARSE_TYPE_NOT_PARSED:
		if (qualifiers != TYPE_QUALIFIER_NONE) {
			return PARSE_TYPE_ERROR;
		}

		return PARSE_TYPE_NOT_PARSED;
	case PARSE_TYPE_ERROR:
		return PARSE_TYPE_ERROR;
	case PARSE_TYPE_PARSED:
		break;
	}

	Token maybe_asterisk = preprocessor_view_next(parser->preprocessor);
	if (maybe_asterisk.kind == TOKEN_ASTERISK) {
		preprocessor_next_token(parser->preprocessor);

		TypeQualifiers qualifiers = _parser_parse_type_qualifiers(parser);

		Type* inner_type = arena_alloc(parser->ast_allocator, Type);
		*inner_type = *out_type;

		*out_type = (Type) {
			.kind = TYPE_POINTER,
			.pointer_base_type = inner_type,
			.qualifiers = qualifiers,
		};
	}

	return PARSE_TYPE_PARSED;
}

static bool _parser_parse_type(Parser* parser, Type* out_type, bool is_anonymous) {
	assert(out_type != NULL);

	// First parse qualifiers
	out_type->qualifiers = _parser_parse_type_qualifiers(parser);

	switch (_parser_try_parse_type_specifier(parser, out_type, is_anonymous)) {
	case PARSE_TYPE_ERROR:
	case PARSE_TYPE_NOT_PARSED:
		return false;
	case PARSE_TYPE_PARSED:
		out_type->qualifiers |= _parser_parse_type_qualifiers(parser);
		return true;
	}

	unreachable();
	return false;
}

AstNode* _parser_parse_type_def(Parser* parser) {
	Token keyword_token = preprocessor_next_token(parser->preprocessor);
	assert(keyword_token.kind == TOKEN_KEYWORD_TYPEDEF);

	Type aliased_type = {};
	if (!_parser_parse_type(parser, &aliased_type, false)) {
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

	TypeDef* type_def = arena_alloc(parser->ast_allocator, TypeDef);
	memset(type_def, 0, sizeof(*type_def));
	
	type_def->new_name = source_string_from_token(new_name);
	type_def->aliased_type = aliased_type;

	IdentifierEntry* entry = ident_storage_find(parser->ident_storage,
			IDENT_NAMESPACE_ALIAS,
			IDENT_FIND_DEFAULT,
			new_name.string);
	if (!entry) {
		entry = ident_storage_insert(parser->ident_storage,
				IDENT_NAMESPACE_ALIAS,
				IDENT_TYPE_DEF,
				source_string_from_token(new_name));
	}

	entry->type_def = type_def;

	AstNode* node = arena_alloc_zeroed(parser->ast_allocator, AstNode);
	node->kind = AST_NODE_TYPE_DEF;
	node->type_def = type_def;
	return node;
}

static bool _parser_parse_function_params(Parser* parser,
		FunctionParam** out_params,
		size_t* out_param_count,
		bool* out_has_va_args) {

	Token left_paren = preprocessor_next_token(parser->preprocessor);
	assert(left_paren.kind == TOKEN_LEFT_PAREN);

	ArenaRegion ast_temp = arena_begin_temp(parser->ast_allocator);
	ArenaRegion temp = arena_begin_temp(parser->temp_allocator);

	FunctionParam* params = arena_alloc_array(parser->temp_allocator, FunctionParam, 0);
	size_t param_count = 0;
	bool has_va_args = false;

	while (true) {
		{
			Token token = preprocessor_view_next(parser->preprocessor);
			if (token.kind == TOKEN_RIGHT_PAREN) {
				preprocessor_next_token(parser->preprocessor);
				break;
			} else if (token.kind == TOKEN_ELLIPSES) {
				preprocessor_next_token(parser->preprocessor);
				has_va_args = true;

				Token right_paren = preprocessor_next_token(parser->preprocessor);
				if (right_paren.kind != TOKEN_RIGHT_PAREN) {
					TokenKind expected_tokens[] = { TOKEN_RIGHT_PAREN };
					diagnostics_report_unexpected_token(parser->diagnostics,
							right_paren,
							expected_tokens,
							array_size(expected_tokens));

					arena_end_temp(ast_temp);
					arena_end_temp(temp);
					return false;
				}

				break;
			}
		}

		FunctionParam* param = arena_alloc_zeroed(parser->temp_allocator, FunctionParam);
		param_count += 1;

		if (!_parser_parse_type(parser, &param->type, true)) {
			arena_end_temp(temp);
			arena_end_temp(ast_temp);
			return false;
		}

		if (!_parser_parse_pre_declaration_modifiers(parser, &param->type, &param->type, true)) {
			arena_end_temp(temp);
			arena_end_temp(ast_temp);
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
			arena_end_temp(ast_temp);
			return false;
		}

		if (token.kind == TOKEN_COMMA) {
			preprocessor_next_token(parser->preprocessor);
			continue; // we most likely have more parameters
		} else if (token.kind == TOKEN_RIGHT_PAREN) {
			preprocessor_next_token(parser->preprocessor);
			break;
		}
	}

	if (param_count == 0) {
		*out_params = NULL;
	} else {
		*out_params = arena_alloc_array(parser->ast_allocator, FunctionParam, param_count);
		array_copy(*out_params, params, param_count);
	}

	*out_param_count = param_count;

	// NOTE: This might still be true is there are no parameters
	*out_has_va_args = has_va_args;

	arena_end_temp(temp);
	return true;
}

//
// Expr
//

bool _token_kind_to_bin_op(TokenKind kind, BinOpKind* out_op) {
#define ret(op) *out_op = op; return true;

	switch (kind) {
	case TOKEN_PLUS: ret(BIN_OP_ADD);
	case TOKEN_MINUS: ret(BIN_OP_SUB);
	case TOKEN_ASTERISK: ret(BIN_OP_MUL);
	case TOKEN_FORWARD_SLASH: ret(BIN_OP_DIV);
	case TOKEN_PERCENT: ret(BIN_OP_MOD);

	case TOKEN_LOGIC_AND: ret(BIN_OP_LOGICAL_AND);
	case TOKEN_LOGIC_OR: ret(BIN_OP_LOGICAL_OR);

	case TOKEN_DOUBLE_EQUAL: ret(BIN_OP_LOGICAL_EQUAL);
	case TOKEN_NOT_EQUAL: ret(BIN_OP_LOGICAL_NOT_EQUAL);
	case TOKEN_LESS: ret(BIN_OP_LOGICAL_LESS);
	case TOKEN_GREATER: ret(BIN_OP_LOGICAL_GREATER);
	case TOKEN_LESS_OR_EQUAL: ret(BIN_OP_LOGICAL_LESS_OR_EQUAL);
	case TOKEN_GREATER_OR_EQUAL: ret(BIN_OP_LOGICAL_GREATER_OR_EQUAL);

	case TOKEN_AMPERSAND: ret(BIN_OP_BITWISE_AND);
	case TOKEN_PIPE: ret(BIN_OP_BITWISE_OR);
	case TOKEN_BITWISE_XOR: ret(BIN_OP_BITWISE_XOR);
	case TOKEN_BITWISE_SHIFT_LEFT: ret(BIN_OP_BITWISE_SHIFT_LEFT);
	case TOKEN_BITWISE_SHIFT_RIGHT: ret(BIN_OP_BITWISE_SHIFT_RIGHT);

	case TOKEN_EQUAL: ret(BIN_OP_ASSIGNMENT);

	case TOKEN_ASSIGNMENT_BY_SUM: ret(BIN_OP_ASSIGNMENT_BY_SUM);
	case TOKEN_ASSIGNMENT_BY_DIFFERENCE: ret(BIN_OP_ASSIGNMENT_BY_DIFFERENCE);
	case TOKEN_ASSIGNMENT_BY_PRODUCT: ret(BIN_OP_ASSIGNMENT_BY_PRODUCT);
	case TOKEN_ASSIGNMENT_BY_QUOTIENT: ret(BIN_OP_ASSIGNMENT_BY_QUOTIENT);
	case TOKEN_ASSIGNMENT_BY_REMAINDER: ret(BIN_OP_ASSIGNMENT_BY_REMAINDER);

	case TOKEN_ASSIGNMENT_BY_BITWISE_AND: ret(BIN_OP_ASSIGNMENT_BY_BITWISE_AND);
	case TOKEN_ASSIGNMENT_BY_BITWISE_OR: ret(BIN_OP_ASSIGNMENT_BY_BITWISE_OR);
	case TOKEN_ASSIGNMENT_BY_BITWISE_XOR: ret(BIN_OP_ASSIGNMENT_BY_BITWISE_XOR);
	case TOKEN_ASSIGNMENT_BY_BITWISE_SHIFT_LEFT: ret(BIN_OP_ASSIGNMENT_BY_BITWISE_SHIFT_LEFT);
	case TOKEN_ASSIGNMENT_BY_BITWISE_SHIFT_RIGHT: ret(BIN_OP_ASSIGNMENT_BY_BITWISE_SHIFT_RIGHT);

	default:
		return false;
	}

#undef ret

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

void _parser_parse_string_literal(Parser* parser, StringLiteral* out_literal) {
	StringBuilder builder = { .arena = parser->ast_allocator };

	while (true) {
		Token string_token = preprocessor_view_next(parser->preprocessor);
		if (string_token.kind != TOKEN_STRING) {
			break;
		}

		preprocessor_next_token(parser->preprocessor);

		const SourceFile* source_file = string_token.source_range.source_file;
		String str_content = sub_str(string_token.string, 1, string_token.string.length - 2);
		parse_escaped_string(&builder, str_content, source_file, parser->diagnostics);
	}

	Expr* size_expr = arena_alloc(parser->ast_allocator, Expr);
	size_expr->kind = EXPR_INTEGER_LITERAL;
	size_expr->int_literal.format = INT_LIT_FMT_DECIMAL;
	size_expr->int_literal.integer_type = TYPE_SIZE_T;
	// size including null-terminator
	size_expr->int_literal.value = builder.string.length + 1;

	out_literal->full_string = builder.string;
	out_literal->array_size_expr = size_expr;

}

static ExprParseResult _parser_try_parse_expr_operand_without_post_fix_operator(Parser* parser, Expr* out_expr) {
	Token token = preprocessor_view_next(parser->preprocessor);

	UnaryOpKind unary_op;
	if (_token_kind_to_unary_pre_op(token.kind, &unary_op)) {
		preprocessor_next_token(parser->preprocessor);

		out_expr->kind = EXPR_UNARY;
		out_expr->unary.operand = arena_alloc(parser->ast_allocator, Expr);
		out_expr->unary.op = unary_op;

		return _parser_try_parse_bin_expr_operand(parser, out_expr->unary.operand);
	}
	
	if (token.kind == TOKEN_IDENT) {
		preprocessor_next_token(parser->preprocessor);

		assert(token.string.length > 0);
		bool is_int_literal = is_digit(token.string.v[0]);
		if (is_int_literal) {
			IntLiteral literal = {};
			bool literal_parsed = parse_int_literal(token, parser->diagnostics, &literal);
			if (!literal_parsed) {
				return EXPR_PARSE_ERROR;
			}

			TypeKind int_type = TYPE_VOID;
			if (literal.has_sufix) {
				switch (literal.sufix_kind) {
				case INT_SUFIX_NONE: {
					assert(literal.sufix_bit_count > 0);
					uint8_t bit_count_index = count_trailing_zeros(literal.sufix_bit_count / 8);
					int_type = TYPE_INT8 + bit_count_index;
					break;
				}
				case INT_SUFIX_U: {
					if (literal.sufix_bit_count > 0) {
						uint8_t bit_count_index = count_trailing_zeros(literal.sufix_bit_count / 8);
						int_type = TYPE_INT8 + bit_count_index;
					} else {
						int_type = TYPE_INT;
					}

					int_type |= TYPE_FLAG_UNSIGNED;
					break;
				}
				case INT_SUFIX_L:
					assert(literal.sufix_bit_count == 0);
					int_type = TYPE_LONG;
					break;
				case INT_SUFIX_UL:
					assert(literal.sufix_bit_count == 0);
					int_type = TYPE_UNSIGNED_LONG;
					break;
				case INT_SUFIX_LL:
					assert(literal.sufix_bit_count == 0);
					int_type = TYPE_LONG_LONG;
					break;
				case INT_SUFIX_ULL:
					assert(literal.sufix_bit_count == 0);
					int_type = TYPE_UNSIGNED_LONG_LONG;
					break;
				}
			} else {
				int_type = TYPE_INT;
			}

			out_expr->kind = EXPR_INTEGER_LITERAL;
			out_expr->int_literal = (IntegerLiteral) {
				.format = literal.format,
				.integer_type = int_type,
				.value = literal.value,
			};

			return EXPR_PARSE_OK;
		}

		IdentifierEntry* entry = ident_storage_find(parser->ident_storage,
				IDENT_NAMESPACE_DEFAULT,
				IDENT_FIND_DEFAULT,
				token.string);
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
		case IDENT_UNION:
		case IDENT_ENUM:
			unreachable();
		case IDENT_ENUM_CONSTANT:
			out_expr->kind = EXPR_ENUM_CONSTANT;
			out_expr->enum_constant.enum_def = entry->enum_constant.enum_def;
			out_expr->enum_constant.variant_index = entry->enum_constant.variant_index;
			return EXPR_PARSE_OK;
		case IDENT_FUNCTION_PARAM:
			out_expr->kind = EXPR_FUNCTION_PARAM;
			out_expr->function_param.function_def = entry->function_param.function_def;
			out_expr->function_param.param_index = entry->function_param.param_index;
			return EXPR_PARSE_OK;
		case IDENT_KIND_MAX:
			unreachable();
		}

		unreachable();
	} else if (token.kind == TOKEN_STRING) {
		// we have a string literal
		_parser_parse_string_literal(parser, &out_expr->string_literal);
		out_expr->kind = EXPR_STRING_LITERAL;
		return EXPR_PARSE_OK;
	} else if (token.kind == TOKEN_CHAR) {
		// we have a char literal

		assert(token.string.length >= 2);
		size_t char_literal_length = token.string.length - 2; // the token includes quotes

		String char_literal = sub_str(token.string, 1, token.string.length - 2);
		if (char_literal_length == 0) {
			diagnostics_report_error(parser->diagnostics,
					token.source_range,
					STR_LIT("Empty character constant"),
					NULL);

			preprocessor_next_token(parser->preprocessor);
			return EXPR_PARSE_ERROR;
		}

		enum {
			CHAR_STATE_NONE,
			CHAR_STATE_OK,
			CHAR_STATE_TOO_LONG,
		} char_state = CHAR_STATE_NONE;

		if (char_literal.length == 1) {
			char_state = CHAR_STATE_OK;
		} else {
			if (char_literal.v[0] == '\\') {
				EscapedChar escaped_char = parse_escaped_char(char_literal,
						token.source_range.source_file,
						parser->diagnostics);

				if (escaped_char.escape_sequence_length == char_literal.length) {
					char_state = CHAR_STATE_OK;
				} else if (escaped_char.escape_sequence_length <= char_literal.length) {
					char_state = CHAR_STATE_TOO_LONG;
				} else {
					unreachable();
				}
			} else {
				char_state = CHAR_STATE_TOO_LONG;
			}
		}

		switch (char_state) {
		case CHAR_STATE_NONE:
			unreachable();
		case CHAR_STATE_OK:
			out_expr->kind = EXPR_CHAR_LITERAL;
			out_expr->char_literal.value = (uint32_t)token.string.v[1];
			preprocessor_next_token(parser->preprocessor);
			return EXPR_PARSE_OK;
		case CHAR_STATE_TOO_LONG:
			diagnostics_report_error(parser->diagnostics,
					token.source_range,
					STR_LIT("Character constant is too long"),
					NULL);

			preprocessor_next_token(parser->preprocessor);
			return EXPR_PARSE_ERROR;
		}

		unreachable();
	} else if (token.kind == TOKEN_LEFT_PAREN) {
		preprocessor_next_token(parser->preprocessor);

		Type cast_target_type;
		ParseTypeResult type_result = _parser_try_parse_type_name(parser, &cast_target_type);
		switch (type_result) {
		case PARSE_TYPE_PARSED: {
			Token right_paren = preprocessor_next_token(parser->preprocessor);
			if (right_paren.kind != TOKEN_RIGHT_PAREN) {
				TokenKind expected_tokens[] = { TOKEN_RIGHT_PAREN };

				diagnostics_report_unexpected_token(parser->diagnostics,
						right_paren,
						expected_tokens,
						array_size(expected_tokens));
				return EXPR_PARSE_ERROR;
			}

			out_expr->kind = EXPR_CAST;
			out_expr->cast.target_type = arena_alloc(parser->ast_allocator, Type);
			out_expr->cast.expr = arena_alloc(parser->ast_allocator, Expr);

			*out_expr->cast.target_type = cast_target_type;

			ExprParseResult expr_result = _parser_try_parse_expr_operand_without_post_fix_operator(
					parser,
					out_expr->cast.expr);

			return expr_result;
		}
		case PARSE_TYPE_NOT_PARSED:
			break;
		case PARSE_TYPE_ERROR:
			return EXPR_PARSE_ERROR;
		}

		ExprParseResult result = _parser_try_parse_expr(parser, out_expr);
		if (result != EXPR_PARSE_OK) {
			return result;
		}

		Token right_paren = preprocessor_next_token(parser->preprocessor);
		if (right_paren.kind != TOKEN_RIGHT_PAREN) {
			TokenKind expected_tokens[] = { TOKEN_RIGHT_PAREN };

			diagnostics_report_unexpected_token(parser->diagnostics,
					right_paren,
					expected_tokens,
					array_size(expected_tokens));
			return EXPR_PARSE_ERROR;
		}

		return EXPR_PARSE_OK;
	}

	return EXPR_PARSE_NOT_PARSED;
}

static ExprParseResult _parser_parse_arg_list(Parser* parser, ExprArray* out_expr_array) {
	Token left_paren = preprocessor_next_token(parser->preprocessor);
	assert(left_paren.kind == TOKEN_LEFT_PAREN);

	ArenaRegion temp = arena_begin_temp(parser->temp_allocator);

	ExprArray args = {};
	args.count = 0;
	args.exprs = arena_alloc_array(parser->temp_allocator, Expr*, 0);

	while (true) {
		Token maybe_right_paren = preprocessor_view_next(parser->preprocessor);
		if (maybe_right_paren.kind == TOKEN_RIGHT_PAREN) {
			preprocessor_next_token(parser->preprocessor);
			break;
		}

		Expr* arg = arena_alloc(parser->ast_allocator, Expr);
		ExprParseResult result = _parser_try_parse_expr(parser, arg);
		if (result != EXPR_PARSE_OK) {
			arena_end_temp(temp);
			return result;
		}

		arena_alloc(parser->temp_allocator, Expr);
		args.exprs[args.count] = arg;
		args.count += 1;

		Token comma_or_right_paren = preprocessor_next_token(parser->preprocessor);
		if (comma_or_right_paren.kind == TOKEN_COMMA) {
			continue;
		} else if (comma_or_right_paren.kind == TOKEN_RIGHT_PAREN) {
			break;
		} else {
			TokenKind expected_tokens[] = { TOKEN_COMMA, TOKEN_RIGHT_PAREN };
			diagnostics_report_unexpected_token(parser->diagnostics,
					comma_or_right_paren,
					expected_tokens,
					array_size(expected_tokens));
			
			arena_end_temp(temp);
			return EXPR_PARSE_ERROR;
		}
	}

	out_expr_array->count = args.count;
	out_expr_array->exprs = arena_alloc_array(parser->ast_allocator, Expr*, args.count);
	memcpy(out_expr_array->exprs, args.exprs, sizeof(*args.exprs) * args.count);

	arena_end_temp(temp);
	return EXPR_PARSE_OK;
}

// Tries to parse an expression operand + any post fix operators,
// like increment, decrement, array access, member access or a call
static ExprParseResult _parser_try_parse_bin_expr_operand(Parser* parser, Expr* out_expr) {
	ExprParseResult result = _parser_try_parse_expr_operand_without_post_fix_operator(parser, out_expr);
	if (result != EXPR_PARSE_OK) {
		return result;
	}

	while (true) {
		Token operator_token = preprocessor_view_next(parser->preprocessor);
		if (operator_token.kind == TOKEN_LEFT_PAREN) {
			// We've got a function call
			
			ExprArray args;
			ExprParseResult result = _parser_parse_arg_list(parser, &args);
			if (result != EXPR_PARSE_OK) {
				return result;
			}

			Expr* callable = arena_alloc(parser->ast_allocator, Expr);
			memcpy(callable, out_expr, sizeof(*out_expr));

			out_expr->kind = EXPR_CALL;
			out_expr->call.callable = callable;
			out_expr->call.args = args;
		} else if (operator_token.kind == TOKEN_LEFT_BRACKET) {
			preprocessor_next_token(parser->preprocessor);

			Expr* array = arena_alloc(parser->ast_allocator, Expr);
			memcpy(array, out_expr, sizeof(*out_expr));

			Expr* index = arena_alloc(parser->ast_allocator, Expr);
			ExprParseResult result = _parser_try_parse_expr(parser, index);
			if (result != EXPR_PARSE_OK) {
				return result;
			}

			out_expr->kind = EXPR_ARRAY_INDEX;
			out_expr->array_index.array = array;
			out_expr->array_index.index = index;

			Token closing_bracket = preprocessor_next_token(parser->preprocessor);
			if (closing_bracket.kind != TOKEN_RIGHT_BRACKET) {
				TokenKind expected_tokens[] = { TOKEN_RIGHT_BRACKET };
				diagnostics_report_unexpected_token(parser->diagnostics,
						closing_bracket,
						expected_tokens,
						array_size(expected_tokens));
				return EXPR_PARSE_ERROR;
			}
		} else {
			break;
		}
	}

	return EXPR_PARSE_OK;
}

ExprParseResult _parser_try_parse_expr(Parser* parser, Expr* out_expr) {
	ExprParseResult left_operand_result = _parser_try_parse_bin_expr_operand(parser, out_expr);
	if (left_operand_result != EXPR_PARSE_OK) {
		return left_operand_result;
	}

	Expr* current_expr = out_expr;

	while (true) {
		Token op_token = preprocessor_view_next(parser->preprocessor);

		BinOpKind current_bin_op;
		if (_token_kind_to_bin_op(op_token.kind, &current_bin_op)) {
			preprocessor_next_token(parser->preprocessor);
				
			Expr* right_operand = arena_alloc(parser->ast_allocator, Expr);

			Token first_operand_token = preprocessor_view_next(parser->preprocessor);
			if (_parser_try_parse_bin_expr_operand(parser, right_operand) != EXPR_PARSE_OK) {
				diagnostics_report_error(parser->diagnostics,
						first_operand_token.source_range,
						STR_LIT("Expected binary expression operand"),
						NULL);
				return EXPR_PARSE_ERROR;
			}

			Expr* left_operand = arena_alloc(parser->ast_allocator, Expr);

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

			Type left_type;
			Type right_type;
			expr_get_type(left_operand, &left_type);
			expr_get_type(right_operand, &right_type);

			if (left_type.kind == TYPE_ENUM) {
				left_type.kind = TYPE_INT;
			}

			if (right_type.kind == TYPE_ENUM) {
				right_type.kind = TYPE_INT;
			}

			Type result_type;
			bin_expr_select_result_type(&left_type, &right_type, &result_type);

			*current_expr = (Expr) {
				.kind = EXPR_BINARY,
				.binary = (BinExpr) {
					.op = current_bin_op,
					.result_type_kind = result_type.kind,
					.pointer_base_type = result_type.pointer_base_type,
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
		Type* base_type,
		Type* out_type,
		bool duplicate_base_type) {
	if (base_type == out_type) {
		assert(duplicate_base_type);
	}

	while (true) {
		Token maybe_asterisk = preprocessor_view_next(parser->preprocessor);
		if (maybe_asterisk.kind == TOKEN_ASTERISK) {
			preprocessor_next_token(parser->preprocessor);

			TypeQualifiers qualifiers = _parser_parse_type_qualifiers(parser);

			assert(duplicate_base_type);

			Type* inner_type = arena_alloc(parser->ast_allocator, Type);
			*inner_type = *base_type;

			*out_type = (Type) {
				.kind = TYPE_POINTER,
				.pointer_base_type = inner_type,
				.qualifiers = qualifiers,
			};
		} else {
			TypeQualifiers qualifiers = _parser_parse_type_qualifiers(parser);
			out_type->qualifiers |= qualifiers;
			break;
		}
	}

	return true;
}

bool _parser_parse_post_declaration_modifiers(Parser* parser,
		Type* base_type,
		Type* out_type,
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

			Expr* size_expr = NULL;
			if (has_size_expr) {
				size_expr = arena_alloc(parser->ast_allocator, Expr);

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

			Type* inner_type = arena_alloc(parser->ast_allocator, Type);
			*inner_type = *base_type;

			*out_type = (Type) {
				.kind = TYPE_ARRAY,
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

bool _check_for_var_redefinition(Parser* parser, SourceString var_name) {
	IdentifierEntry* existing_identifier = ident_storage_find(parser->ident_storage,
			IDENT_NAMESPACE_DEFAULT,
			IDENT_FIND_IN_CURRENT_SCOPE,
			var_name.string);

	if (existing_identifier != NULL) {
		StringBuilder builder = { .arena = parser->diagnostics->allocator };
		str_builder_append(&builder, STR_LIT("Redefinition of '"));
		str_builder_append(&builder, var_name.string);
		str_builder_append_char(&builder, '\'');

		DiagnosticsEntry* error = diagnostics_report_error(parser->diagnostics,
				source_string_to_range(var_name),
				builder.string,
				NULL);

		diagnostics_report_error(parser->diagnostics,
				source_string_to_range(existing_identifier->name),
				STR_LIT("Previously defined here"),
				error);
		return false;
	} 

	return true;
}

static void _parser_register_function_param_identifiers(Parser* parser, Function* function_def) {
	assert(!function_def->is_forward_declared);

	for (size_t i = 0; i < function_def->parameter_count; i += 1) {
		const FunctionParam* param = &function_def->parameters[i];
		if (param->name.string.length == 0) {
			continue;
		}

		IdentifierEntry* entry = ident_storage_find(parser->ident_storage,
				IDENT_NAMESPACE_DEFAULT,
				IDENT_FIND_DEFAULT,
				param->name.string);

		if (entry) {
			StringBuilder builder = { .arena = parser->diagnostics->allocator };
			str_builder_append(&builder, STR_LIT("Name \'"));
			str_builder_append(&builder, entry->name.string);
			str_builder_append(&builder, STR_LIT("' is already defined"));

			DiagnosticsEntry* error = diagnostics_report_error(parser->diagnostics,
					source_string_to_range(param->name),
					builder.string,
					NULL);

			diagnostics_report_error(parser->diagnostics,
					source_string_to_range(entry->name),
					STR_LIT("Previously defined here"),
					error);
		} else {
			entry = ident_storage_insert(parser->ident_storage,
					IDENT_NAMESPACE_DEFAULT,
					IDENT_FUNCTION_PARAM,
					param->name);

			entry->function_param.function_def = function_def;
			entry->function_param.param_index = i;
		}
	}
}

static AstNode* _parser_parse_function_declaration(Parser* parser,
		SourceString name,
		Type* return_type,
		DeclSpec* decl_spec,
		StorageSpecifier storage_specifier,
		FunctionCallingConvention call_conv) {
	Token token = preprocessor_view_next(parser->preprocessor);
	assert(token.kind == TOKEN_LEFT_PAREN);

	FunctionParam* params = NULL;
	size_t param_count = 0;
	bool has_va_args = false;
	if (!_parser_parse_function_params(parser, &params, &param_count, &has_va_args)) {
		return NULL;
	}

	// Register the declaration
	Function* function_def = NULL;
	IdentifierEntry* entry = ident_storage_find(parser->ident_storage,
			IDENT_NAMESPACE_DEFAULT,
			IDENT_FIND_DEFAULT,
			name.string);
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
			return NULL;
		}

		function_def = entry->function_def;
		assert(function_def);

		// TODO: Verify that return types also match
		if (function_def->parameter_count != param_count || function_def->has_va_args != has_va_args) {
			DiagnosticsEntry* error = diagnostics_report_error(parser->diagnostics,
					source_string_to_range(name),
					STR_LIT("Function was previously defined with a different parameter count"),
					NULL);

			diagnostics_report_error(parser->diagnostics,
					source_string_to_range(entry->name),
					STR_LIT("Previously defined here"),
					error);
			return NULL;
		} else {
			FunctionParam* prev_def_param = function_def->parameters;
			FunctionParam* new_def_param = params;

			for (size_t i = 0; i < param_count; i += 1) {
				bool param_types_are_equal = type_equal(&prev_def_param->type, &new_def_param->type);

				if (!param_types_are_equal) {
					DiagnosticsEntry* error = diagnostics_report_error(parser->diagnostics,
						source_string_to_range(new_def_param->name),
						STR_LIT("Function previously defined with different parameter types"),
						NULL);

					diagnostics_report_error(parser->diagnostics,
							source_string_to_range(prev_def_param->name),
							STR_LIT("Previously defined here"),
							error);
					return NULL;
				}
				
				prev_def_param = prev_def_param + 1;
				new_def_param = new_def_param + 1;
			}
		}
	} else {
		entry = ident_storage_insert(parser->ident_storage,
				IDENT_NAMESPACE_DEFAULT,
				IDENT_FUNCTION,
				name);

		function_def = arena_alloc_zeroed(parser->ast_allocator, Function); 
		
		function_def->name = name;
		function_def->return_type = *return_type;
		function_def->parameters = params;
		function_def->parameter_count = param_count;
		function_def->is_forward_declared = true;
		function_def->decl_spec = decl_spec;
		function_def->storage_specifier = storage_specifier;
		function_def->var_count = 0;
		function_def->has_va_args = has_va_args;

		entry->function_def = function_def;
	}

	// Check whether the function has a body
	bool has_body = false;
	Token left_brace_or_semicolon = preprocessor_view_next(parser->preprocessor);
	switch (left_brace_or_semicolon.kind) {
	case TOKEN_LEFT_BRACE:
		has_body = true;
		break;
	case TOKEN_SEMICOLON:
		has_body = false;
		preprocessor_next_token(parser->preprocessor);
		break;
	default: {
		TokenKind expected_tokens[] = { TOKEN_LEFT_BRACE, TOKEN_SEMICOLON };
		diagnostics_report_unexpected_token(parser->diagnostics,
				left_brace_or_semicolon,
				expected_tokens,
				array_size(expected_tokens));
		return NULL;
	}
	}

	// Check for redefinition
	if (!function_def->is_forward_declared && has_body) {
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
		return NULL;
	}

	// Parse the body
	if (has_body) {
		assert(function_def->is_forward_declared);
		function_def->is_forward_declared = false;

		Scope* body = arena_alloc(parser->ast_allocator, Scope);
		memset(body, 0, sizeof(*body));

		ident_storage_begin_scope(parser->ident_storage);
		_parser_register_function_param_identifiers(parser, function_def);

		uint32_t last_var_id_state = parser->next_var_id;

		bool result = _parser_parse_scope(parser, body);

		uint32_t var_count = parser->next_var_id - last_var_id_state;
		parser->next_var_id = last_var_id_state;

		ident_storage_end_scope(parser->ident_storage);

		if (!result) {
			return NULL;
		}

		function_def->body = body;
		function_def->var_count = var_count;
	}

	AstNode* node = arena_alloc_zeroed(parser->ast_allocator, AstNode);
	node->kind = AST_NODE_FUNCTION;
	node->function_def = function_def;
	return node;
}

AstNode* _parser_parse_type_declaration(Parser* parser,
		Type* type,
		DeclSpec* decl_spec,
		StorageSpecifier storage_specifier) {

	assert(type != NULL);

	if (!_parser_parse_pre_declaration_modifiers(parser, type, type, true)) {
		return NULL;
	}

	FunctionCallingConvention call_conv = FUNC_CALL_CONV_DEFAULT;

	Token name_token = preprocessor_next_token(parser->preprocessor);
	if (name_token.kind != TOKEN_IDENT) {
		TokenKind expected_tokens[] = { TOKEN_IDENT };
		diagnostics_report_unexpected_token(parser->diagnostics,
				name_token,
				expected_tokens,
				array_size(expected_tokens));
		
		return NULL;
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
			
			return NULL;
		}
	}

	SourceString name = source_string_from_token(name_token);

	Token token = preprocessor_view_next(parser->preprocessor);
	if (token.kind == TOKEN_LEFT_PAREN) {
		return _parser_parse_function_declaration(parser, name, type, decl_spec, storage_specifier, call_conv);
	}

	if (token.kind == TOKEN_LEFT_BRACKET) {
		// NOTE: This is a bit overcomplicated.
		//       Just extract function handling into a separate function,
		//       and simplify the logic
		if (!_parser_parse_post_declaration_modifiers(parser, type, type, true)) {
			return NULL;
		}

		token = preprocessor_view_next(parser->preprocessor);
	}

	if (token.kind == TOKEN_EQUAL) {
		preprocessor_next_token(parser->preprocessor);

		assert(decl_spec == NULL);

		Expr* value = arena_alloc(parser->ast_allocator, Expr);
		switch (_parser_try_parse_expr(parser, value)) {
		case EXPR_PARSE_OK:
			break;
		case EXPR_PARSE_ERROR:
			return NULL;
		case EXPR_PARSE_NOT_PARSED:
			diagnostics_report_error(parser->diagnostics,
					token.source_range,
					STR_LIT("Expected variable value after the '='"),
					NULL);
			return NULL;
		}

		if (!_parser_expect_semicolon(parser, STR_LIT("Expected semicolon after the variable definition"))) {
			return NULL;
		}

		if (!_check_for_var_redefinition(parser, name)) {
			return NULL;
		}

		AstNode* node = arena_alloc_zeroed(parser->ast_allocator, AstNode);
		node->kind = AST_NODE_VARIABLE;
		node->variable.name = name;
		node->variable.type = *type;
		node->variable.value = value;
		node->variable.storage_specifier = storage_specifier;
		node->variable.id = parser->next_var_id;

		parser->next_var_id += 1;

		IdentifierEntry* entry = ident_storage_insert(parser->ident_storage,
				IDENT_NAMESPACE_DEFAULT,
				IDENT_VARIABLE,
				node->variable.name);

		entry->variable = &node->variable;
		return node;
	} else if (token.kind == TOKEN_SEMICOLON) {
		preprocessor_next_token(parser->preprocessor);

		assert(decl_spec == NULL);

		if (!_check_for_var_redefinition(parser, name)) {
			return NULL;
		}

		// A variable declaration
		AstNode* node = arena_alloc_zeroed(parser->ast_allocator, AstNode);
		node->kind = AST_NODE_VARIABLE;
		node->variable = (Variable) {
			.name = name,
			.type = *type,
			.value = NULL,
			.storage_specifier = storage_specifier,
			.id = parser->next_var_id,
		};

		parser->next_var_id += 1;

		IdentifierEntry* entry = ident_storage_insert(parser->ident_storage,
				IDENT_NAMESPACE_DEFAULT,
				IDENT_VARIABLE,
				name);

		entry->variable = &node->variable;
		return node;
	} else {
		TokenKind expected_tokens[] = {
			TOKEN_LEFT_PAREN,
			TOKEN_EQUAL,
			TOKEN_SEMICOLON,
			TOKEN_LEFT_BRACKET
		};

		diagnostics_report_unexpected_token(parser->diagnostics,
				token,
				expected_tokens,
				array_size(expected_tokens));
		return NULL;
	}

	unreachable();
	return NULL;
}

AstNode* _parser_parse_variable_or_function_def(Parser* parser,
		DeclSpec* decl_spec,
		StorageSpecifier storage_specifier) {

	bool has_type = false;
	TypeQualifiers type_qualifiers = _parser_parse_type_qualifiers(parser);
	Type type = { .qualifiers = type_qualifiers };

	Token maybe_type_specifier_token = preprocessor_view_next(parser->preprocessor);

	switch (_parser_try_parse_type_specifier(parser, &type, true)) {
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

			return NULL;
		}

		break;
	case PARSE_TYPE_ERROR:
		return NULL;
	}

	type.qualifiers |= _parser_parse_type_qualifiers(parser);

	if (has_type) {
		return _parser_parse_type_declaration(parser, &type, decl_spec, storage_specifier);
	} else {
		if (decl_spec) {
			debug_log_info("__declspec ignore before expression");
		}

		if (storage_specifier != STORAGE_SPEC_NONE) {
			debug_log_info("storage specifier skipped before expression");
		}

		Expr expr;
		ExprParseResult result = _parser_try_parse_expr(parser, &expr);
		if (result == EXPR_PARSE_OK) {
			if (!_parser_expect_semicolon(parser, STR_LIT("Expected ';' after an expression"))) {
				return NULL;
			}

			AstNode* node = arena_alloc_zeroed(parser->ast_allocator, AstNode);
			node->kind = AST_NODE_EXPR;
			node->expr = expr;

			return node;
		} else {
			return NULL;
		}
	}

	unreachable();
	return NULL;
}

static DeclSpec* _parser_parse_decl_spec(Parser* parser) {
	Token decl_spec_token = preprocessor_view_next(parser->preprocessor);
	if (decl_spec_token.kind != TOKEN_DECLSPEC) {
		return NULL;
	}

	preprocessor_next_token(parser->preprocessor);

	Token left_paren = preprocessor_next_token(parser->preprocessor);
	if (left_paren.kind != TOKEN_LEFT_PAREN) {
		TokenKind expected_tokens[] = { TOKEN_LEFT_PAREN };
		diagnostics_report_unexpected_token(parser->diagnostics,
				left_paren,
				expected_tokens,
				array_size(expected_tokens));
		return NULL;
	}

	Token token = preprocessor_next_token(parser->preprocessor);

	DeclSpecKind kind = -1;
	if (str_equal(token.string, STR_LIT("deprecated"))) {
		kind = DECL_SPEC_DEPRECATED;
	} else if (str_equal(token.string, STR_LIT("noinline"))) {
		kind = DECL_SPEC_NO_INLINE;
	} else if (str_equal(token.string, STR_LIT("noreturn"))) {
		kind = DECL_SPEC_NO_RETURN;
	} else if (str_equal(token.string, STR_LIT("dllimport"))) {
		kind = DECL_SPEC_DLL_IMPORT;
	} else if (str_equal(token.string, STR_LIT("dllexport"))) {
		kind = DECL_SPEC_DLL_EXPORT;
	} else if (str_equal(token.string, STR_LIT("restrict"))) {
		kind = DECL_SPEC_RESTRICT;
	}

	if (kind == -1) {
		diagnostics_report_error(parser->diagnostics,
				token.source_range,
				STR_LIT("Unsupported __declspec modifier"),
				NULL);
		return NULL;
	}

	StringLiteral deprecation_text;

	if (kind == DECL_SPEC_DEPRECATED) {
		Token left_paren = preprocessor_next_token(parser->preprocessor);
		if (left_paren.kind != TOKEN_LEFT_PAREN) {
			TokenKind expected_tokens[] = { TOKEN_LEFT_PAREN };
			diagnostics_report_unexpected_token(parser->diagnostics,
					left_paren,
					expected_tokens,
					array_size(expected_tokens));
			return NULL;
		}

		_parser_parse_string_literal(parser, &deprecation_text);

		Token right_paren = preprocessor_next_token(parser->preprocessor);
		if (right_paren.kind != TOKEN_RIGHT_PAREN) {
			TokenKind expected_tokens[] = { TOKEN_RIGHT_PAREN };
			diagnostics_report_unexpected_token(parser->diagnostics,
					right_paren,
					expected_tokens,
					array_size(expected_tokens));
			return NULL;
		}
	}

	Token right_paren = preprocessor_next_token(parser->preprocessor);
	if (right_paren.kind != TOKEN_RIGHT_PAREN) {
		TokenKind expected_tokens[] = { TOKEN_RIGHT_PAREN };
		diagnostics_report_unexpected_token(parser->diagnostics,
				right_paren,
				expected_tokens,
				array_size(expected_tokens));
		return NULL;
	}

	DeclSpec* decl_spec = arena_alloc(parser->ast_allocator, DeclSpec);
	decl_spec->kind = kind;

	if (kind == DECL_SPEC_DEPRECATED) {
		decl_spec->deprecation_text = deprecation_text;
	}

	return decl_spec;
}

static AstNode* _parser_parse_if_stmt(Parser* parser) {
	Token if_token = preprocessor_next_token(parser->preprocessor);
	assert(if_token.kind == TOKEN_KEYWORD_IF);

	Token left_paren = preprocessor_next_token(parser->preprocessor);
	if (left_paren.kind != TOKEN_LEFT_PAREN) {
		TokenKind expected_tokens[] = { TOKEN_LEFT_PAREN };
		diagnostics_report_unexpected_token(parser->diagnostics,
				left_paren,
				expected_tokens,
				array_size(expected_tokens));
		return NULL;
	}

	Expr condition = {};
	if (_parser_try_parse_expr(parser, &condition) != EXPR_PARSE_OK) {
		return NULL;
	}
	
	Token right_paren = preprocessor_next_token(parser->preprocessor);
	if (right_paren.kind != TOKEN_RIGHT_PAREN) {
		TokenKind expected_tokens[] = { TOKEN_RIGHT_PAREN };
		diagnostics_report_unexpected_token(parser->diagnostics,
				right_paren,
				expected_tokens,
				array_size(expected_tokens));
		return NULL;
	}

	AstNode* true_node = NULL;
	AstNode* false_node = NULL;

	Token true_node_token = preprocessor_view_next(parser->preprocessor);

	{
		ident_storage_begin_scope(parser->ident_storage);
		true_node = _parser_parse_single_node(parser, true_node_token);
		ident_storage_end_scope(parser->ident_storage);
	}

	if (true_node == NULL) {
		diagnostics_report_error(parser->diagnostics,
				true_node_token.source_range,
				STR_LIT("Expected a statement if condition"),
				NULL);
		return NULL;
	}

	Token maybe_else = preprocessor_view_next(parser->preprocessor);
	if (maybe_else.kind == TOKEN_KEYWORD_ELSE) {
		preprocessor_next_token(parser->preprocessor);

		Token false_node_token = preprocessor_view_next(parser->preprocessor);

		{
			ident_storage_begin_scope(parser->ident_storage);
			false_node = _parser_parse_single_node(parser, false_node_token);
			ident_storage_end_scope(parser->ident_storage);
		}

		if (false_node == NULL) {
			diagnostics_report_error(parser->diagnostics,
					false_node_token.source_range,
					STR_LIT("Expected a statement after else"),
					NULL);
			return NULL;
		}
	}

	AstNode* if_stmt_node = arena_alloc_zeroed(parser->ast_allocator, AstNode);
	if_stmt_node->kind = AST_NODE_IF;
	if_stmt_node->if_stmt.condition = condition;
	if_stmt_node->if_stmt.true_node = true_node;
	if_stmt_node->if_stmt.false_node = false_node;
	return if_stmt_node;
}

AstNode* _parser_parse_single_node(Parser* parser, Token initial_token) {
	switch (initial_token.kind) {
	case TOKEN_LEFT_BRACE: {
		Scope scope = {};
		if (!_parser_parse_scope(parser, &scope)) {
			return NULL;
		}

		AstNode* node = arena_alloc_zeroed(parser->ast_allocator, AstNode);
		node->kind = AST_NODE_BLOCK;
		node->block = scope;
		return node;
	}
	case TOKEN_KEYWORD_TYPEDEF: {
		return _parser_parse_type_def(parser);
	}
	case TOKEN_KEYWORD_STRUCT: {
		Struct* struct_def = NULL;
		if (!_parser_parse_struct_def(parser, &struct_def, false)) {
			return NULL;
		}

		assert(struct_def->layout_kind == STRUCT_LAYOUT_KIND_STRUCT);

		if (!_parser_expect_semicolon(parser, STR_LIT("Expected ';' after the struct"))) {
			return NULL;
		}

		AstNode* node = arena_alloc_zeroed(parser->ast_allocator, AstNode);

		node->kind = AST_NODE_STRUCT;
		node->struct_def = struct_def;
		return node;
	}
	case TOKEN_KEYWORD_UNION: {
		Struct* union_def = NULL;
		if (!_parser_parse_struct_def(parser, &union_def, false)) {
			return NULL;
		}

		assert(union_def->layout_kind == STRUCT_LAYOUT_KIND_UNION);

		if (!_parser_expect_semicolon(parser, STR_LIT("Expected ';' after the struct"))) {
			return NULL;
		}

		AstNode* node = arena_alloc_zeroed(parser->ast_allocator, AstNode);

		node->kind = AST_NODE_STRUCT;
		node->union_def = union_def;
		return node;
	}
	case TOKEN_KEYWORD_ENUM: {
		Enum* enum_def = NULL;

		if (!_parser_parse_enum_def(parser, &enum_def, false)) {
			return NULL;
		}

		if (!_parser_expect_semicolon(parser, STR_LIT("Expected ';' after the enum"))) {
			return NULL;
		}

		AstNode* node = arena_alloc_zeroed(parser->ast_allocator, AstNode);
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
		
		Expr* return_value = NULL;
		if (has_value) {
			return_value = arena_alloc(parser->ast_allocator, Expr);

			if (_parser_try_parse_expr(parser, return_value) != EXPR_PARSE_OK) {
				return NULL;
			}
		}

		if (!_parser_expect_semicolon(parser, STR_LIT("Expected ';' after the return"))) {
			return NULL;
		}

		AstNode* node = arena_alloc_zeroed(parser->ast_allocator, AstNode);
		memset(node, 0, sizeof(*node));

		node->kind = AST_NODE_RETURN;
		node->return_stmt.value = return_value;
		return node;
	}
	case TOKEN_KEYWORD_IF: {
		return _parser_parse_if_stmt(parser);
	}
	default: {
		DeclSpec* decl_spec = _parser_parse_decl_spec(parser);
		StorageSpecifier storage_specifier = STORAGE_SPEC_NONE;

		Token maybe_storage_specifier = preprocessor_view_next(parser->preprocessor);
		if (maybe_storage_specifier.kind == TOKEN_KEYWORD_STATIC) {
			preprocessor_next_token(parser->preprocessor);
			storage_specifier = STORAGE_SPEC_STATIC;
		} else if (maybe_storage_specifier.kind == TOKEN_KEYWORD_EXTERN) {
			preprocessor_next_token(parser->preprocessor);
			storage_specifier = STORAGE_SPEC_EXTERNAL;
		}

		return _parser_parse_variable_or_function_def(parser, decl_spec, storage_specifier);
	}
	}

	unreachable();
	return NULL;
}

bool _parser_parse_scope(Parser* parser, Scope* out_scope) {
	ident_storage_begin_scope(parser->ident_storage);
	out_scope->id = parser->ident_storage->current_scope->id;

	Token token = preprocessor_next_token(parser->preprocessor);
	assert(token.kind == TOKEN_LEFT_BRACE);

	while (true) {
		Token token = preprocessor_view_next(parser->preprocessor);
		if (token.kind == TOKEN_EOF) {
			diagnostics_report_error(parser->diagnostics,
					token.source_range,
					STR_LIT("Unexpected end of file"),
					NULL);

			ident_storage_end_scope(parser->ident_storage);
			return false;
		} else if (token.kind == TOKEN_RIGHT_BRACE) {
			preprocessor_next_token(parser->preprocessor);
			break;
		} else if (token.kind == TOKEN_SEMICOLON) {
			preprocessor_next_token(parser->preprocessor);
			continue;
		}

		AstNode* node = _parser_parse_single_node(parser, token);
		if (node) {
			parsed_node_list_append(&out_scope->nodes, node);
			node->parent_scope = out_scope;
		} else {
			preprocessor_next_token(parser->preprocessor);
		}
	}

	ident_storage_end_scope(parser->ident_storage);

	return true;
}

void parser_init(Parser* parser,
		Arena* ast_allocator,
		Arena* temp_allocator,
		IdentifierStorage* ident_storage,
		Preprocessor* preprocessor,
		Diagnostics* diagnostics) {
	parser->ast_allocator = ast_allocator;
	parser->temp_allocator = temp_allocator;
	parser->diagnostics = diagnostics;
	parser->preprocessor = preprocessor;
	parser->ident_storage = ident_storage;
}

void parser_parse(Parser* parser, AST* ast) {
	ast->root_nodes = (NodeList) {};

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

		AstNode* node = _parser_parse_single_node(parser, token);
		if (node) {
			parsed_node_list_append(&ast->root_nodes, node);

			// NOTE: We don't a root scope, it is a just a list of nodes,
			//       thus we can't assign a parent scope for each node
		} else {
			preprocessor_next_token(parser->preprocessor);
		}
	}
}
