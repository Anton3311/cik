#include "core/core.h"
#include "core/profiler.h"
#include "parser/preprocessor.h"
#include "parser/parser.h"

//
// Common Generator Code
//

typedef struct {
	Arena* allocator;
	Arena* temp_allocator;
	Arena* diagnostics_allocator;
	Arena* generated_tokens_allocator;
} GenContext;

typedef bool(*GeneratorFunction)(GenContext* context);

typedef struct {
	String name;
	GeneratorFunction function;
} GeneratorDescriptor;

bool _parse(GenContext* context,
		String file_path,
		StringArray include_dirs,
		ParsedAST* out_ast,
		IdentifierStorage* out_ident_storage) {

	// Initialize all the required stages

	SourceStorage* source_storage = arena_alloc_zeroed(context->allocator, SourceStorage);
	source_storage_init(source_storage,
			include_dirs,
			context->allocator);

	SourceFile* source_file = source_storage_append_from_path(source_storage,
			STR_LIT("code_gen/src/code_gen/instr.h"),
			context->temp_allocator);

	Diagnostics diagnostics = (Diagnostics) {
		.allocator = context->diagnostics_allocator,
	};

	Preprocessor preprocessor = {};
	preprocessor_init(&preprocessor,
			source_storage,
			source_file,
			&diagnostics,
			context->allocator,
			context->temp_allocator,
			context->generated_tokens_allocator);

	{
		MacroDefinition code_generation_pass = {
			.name = (SourceString) {
				.string = STR_LIT("CODE_GENERATION_PASS"),
				.source_file = source_file,
			},
			.builtin_kind = BUILTIN_MACRO_STDC,
			.token_count = 1,
		};

		macro_table_append(&preprocessor.macro_table, &code_generation_pass);
	}

	Arena ident_arena = arena_alloc_sub_arena(context->allocator, 32 * 1024);
	Arena ast_arena = arena_alloc_sub_arena(context->allocator, 32 * 1024);

	ident_storage_init(out_ident_storage, heap_allocator_new(), &ident_arena);

	Parser parser = {};
	parser_init(&parser, &ast_arena, context->temp_allocator, out_ident_storage, &preprocessor, &diagnostics);

	// Parse

	parser_parse(&parser, out_ast);

	return diagnostics.first == NULL;
}

void _emit_generated_notice(StringBuilder* builder, const char* generator_function_name) {
	str_builder_append(builder, STR_LIT(
				"//           THIS FILE WAS GENERATED\n"
				"// ANY MANUAL MODIFICATIONS WILL BE DESCARDED\n"
				"//\n"
				"// The generator is implemented in:\n"
				"// File: " __FILE__ "\n"
				"// Function: "));
	str_builder_append_cstr(builder, generator_function_name);
	str_builder_append(builder, STR_LIT("\n\n"));
}

void _emit_include(StringBuilder* builder, String include_path) {
	str_builder_append(builder, STR_LIT("#include \""));
	str_builder_append(builder, include_path);
	str_builder_append(builder, STR_LIT("\"\n"));
}

void _emit_enum_to_string_mapping(GenContext* context,
		StringBuilder* builder,
		String mapping_name,
		const ParsedEnum* enum_def,
		bool skip_last,
		size_t prefix_length) {

	str_builder_append(builder, STR_LIT("static String "));
	str_builder_append(builder, mapping_name);
	str_builder_append(builder, STR_LIT("[] = {\n"));

	size_t variant_count = skip_last ? (enum_def->variant_count - 1) : enum_def->variant_count;
	for (size_t i = 0; i < variant_count; i += 1) {
		String variant_name = enum_def->variants[i].name.string;
		String simple_name = sub_str(variant_name, prefix_length, variant_name.length - prefix_length);

		str_builder_append(builder, STR_LIT("    ["));
		str_builder_append(builder, variant_name);
		str_builder_append(builder, STR_LIT("] = STR_LIT(\""));
		str_builder_append(builder, str_to_lower(simple_name, context->temp_allocator));
		str_builder_append(builder, STR_LIT("\"),\n"));
	}

	str_builder_append(builder, STR_LIT("};\n"));
}

//
// Instr Generator
//

// Maps enum variant to the correspodning field in the `Instr` struct.
// There are specifal cases, because such instruction kinds like `INSTR_BIN_OP_*`
// are defined for different bit counts, are however implemented by the same union
// variant in the `Instr` struct.
static String _instr_kind_to_corresponding_instr_field(const ParsedEnum* enum_def,
		size_t variant_index,
		Arena* temp_allocator) {

	size_t instr_name_prefix_length = sizeof("INSTR_") - 1; // -1 for the null terminator
	String variant_name = enum_def->variants[variant_index].name.string;
	if (str_starts_with(variant_name, STR_LIT("INSTR_BIN_OP_"))) {
		return STR_LIT("bin_op");
	} else if (str_starts_with(variant_name, STR_LIT("INSTR_PTR_LOAD_"))) {
		return STR_LIT("ptr_load");
	} else if (str_starts_with(variant_name, STR_LIT("INSTR_LOGICAL_SHIFT_LEFT_"))) {
		return STR_LIT("logical_shift");
	} else if (str_starts_with(variant_name, STR_LIT("INSTR_LOGICAL_SHIFT_RIGHT_"))) {
		return STR_LIT("logical_shift");
	} else if (str_starts_with(variant_name, STR_LIT("INSTR_COMPARE_"))) {
		return STR_LIT("compare");
	}

	return str_to_lower(
			sub_str(variant_name,
				instr_name_prefix_length,
				variant_name.length - instr_name_prefix_length),
			temp_allocator);
}

static void* _find_type(IdentifierStorage* ident_storage, IdentifierEntryKind kind, String name) {
	assert(kind == IDENT_STRUCT || kind == IDENT_ENUM || kind == IDENT_UNION);
	assert(name.length > 0);

	ParsedTypeKind type_kind = 0;
	switch (kind) {
	case IDENT_STRUCT:
		type_kind = PARSED_TYPE_STRUCT;
		break;
	case IDENT_ENUM:
		type_kind = PARSED_TYPE_ENUM;
		break;
	case IDENT_UNION:
		type_kind = PARSED_TYPE_UNION;
		break;
	default:
		unreachable();
	}

	const IdentifierEntry* alias_entry = ident_storage_find(ident_storage,
			IDENT_NAMESPACE_ALIAS,
			IDENT_FIND_DEFAULT,
			name);

	if (alias_entry) {
		ParsedType* aliased_type = &alias_entry->type_def->aliased_type;
		assert_msg(aliased_type->kind == type_kind,
				"Given type name ('%.*s') is of a different type, that it was expected",
				STR_FMT(name));

		switch (kind) {
		case IDENT_STRUCT:
			return aliased_type->struct_def;
		case IDENT_ENUM:
			return aliased_type->enum_def;
		case IDENT_UNION:
			return aliased_type->union_def;
		default:
			unreachable();
		}
	}

	IdentifierEntry* entry = ident_storage_find(ident_storage,
			IDENT_NAMESPACE_TAGGED,
			IDENT_FIND_DEFAULT,
			name);

	assert_msg(entry != NULL, "Type named %.*s is not found", STR_FMT(name));
	assert_msg(entry->kind == kind, "Type is defined with a different tag");
	
	switch (kind) {
	case IDENT_STRUCT:
		return entry->struct_def;
	case IDENT_ENUM:
		return entry->enum_def;
	case IDENT_UNION:
		return entry->union_def;
	default:
		unreachable();
	}

	unreachable();
	return NULL;
}

bool _generate_instr(GenContext* context) {
	String include_dirs[] = {
		STR_LIT("stdx"),
	};

	ParsedAST ast;
	IdentifierStorage ident_storage;
	if (!_parse(context,
				STR_LIT("code_gen/src/code_gen/inst.h"),
				(StringArray) { .values = include_dirs, .count = array_size(include_dirs) },
				&ast,
				&ident_storage)) {
		return false;
	}

	const ParsedStruct* instr_struct = (const ParsedStruct*)_find_type(&ident_storage,
			IDENT_STRUCT,
			STR_LIT("Instr"));
	const ParsedStruct* instr_index_struct = (const ParsedStruct*)_find_type(&ident_storage,
			IDENT_STRUCT,
			STR_LIT("InstrIndex"));
	const ParsedEnum* instr_kind_enum = (const ParsedEnum*)_find_type(&ident_storage,
			IDENT_ENUM,
			STR_LIT("InstrKind"));
	const ParsedEnum* instr_bin_op_enum = (const ParsedEnum*)_find_type(&ident_storage,
			IDENT_ENUM,
			STR_LIT("InstrBinOp"));
	const ParsedEnum* compare_kind_enum = (const ParsedEnum*)_find_type(&ident_storage,
			IDENT_ENUM,
			STR_LIT("InstrCompareKind"));
	const ParsedStruct* string_struct = (const ParsedStruct*)_find_type(&ident_storage,
			IDENT_STRUCT,
			STR_LIT("String"));

	assert(instr_kind_enum);
	assert(string_struct);

	StringBuilder builder = { .arena = context->allocator };
	_emit_generated_notice(&builder, __func__);

	_emit_include(&builder, STR_LIT("instr.h"));

	size_t instr_name_prefix_length = sizeof("INSTR_") - 1; // -1 for the null terminator

	_emit_enum_to_string_mapping(context,
			&builder,
			STR_LIT("s_instr_kind_to_string"),
			instr_kind_enum,
			true, instr_name_prefix_length);
	_emit_enum_to_string_mapping(context,
			&builder,
			STR_LIT("s_instr_bin_op_kind_to_string"),
			instr_bin_op_enum,
			false,
			sizeof("INSTR_BIN_") - 1);
	_emit_enum_to_string_mapping(context,
			&builder,
			STR_LIT("s_instr_compare_kind_to_string"),
			compare_kind_enum,
			false,
			sizeof("INSTR_CMP_") - 1);

	str_builder_append(&builder, STR_LIT(
				"String instr_name(InstrKind instr_kind) {\n"
				"	return s_instr_kind_to_string[instr_kind];\n"
				"}\n"));
	str_builder_append(&builder, STR_LIT(
				"String instr_bin_op_name(InstrBinOp op_kind) {\n"
				"	return s_instr_bin_op_kind_to_string[op_kind];\n"
				"}\n"));
	str_builder_append(&builder, STR_LIT(
				"String instr_compare_kind_name(InstrCompareKind kind) {\n"
				"	return s_instr_compare_kind_to_string[kind];\n"
				"}\n"));

	// `instr_enumerate_dependencies` is reponsible for pushing every dependency
	// of the current instruction onto the stack,
	// All the fields of type `InstrIndex` store a dependency, so the generator
	// needs to go over all of them, and for each generate a piece of code that
	// pushes dependency stored in that field onto the stack
	str_builder_append(&builder, STR_LIT(
				"void instr_enumerate_dependencies(const InstrBuffer buffer,\n"
				"                                  InstrIndex instr_index,\n"
				"                                  InstrStack* out_dependencies) {\n"
				"    const Instr* instr = &buffer.instr[instr_index.value];\n"
				"    switch (instr->kind) {\n"));

	for (size_t i = 0; i < instr_kind_enum->variant_count - 1; i += 1) {
		String instr_struct_name = _instr_kind_to_corresponding_instr_field(instr_kind_enum, i, context->temp_allocator);

		str_builder_append(&builder, STR_LIT("    case "));
		str_builder_append(&builder, instr_kind_enum->variants[i].name.string);
		str_builder_append(&builder, STR_LIT(":\n"));

		const ParsedStructField* field = struct_find_field(instr_struct, instr_struct_name);
		if (field) {
			switch (field->type.kind) {
			case PARSED_TYPE_STRUCT:
			case PARSED_TYPE_UNION: {
				const ParsedStruct* instr_struct = field->type.struct_def;
				for (size_t j = 0; j < instr_struct->field_count; j += 1) {
					if (!type_is_struct(&instr_struct->fields[j].type, instr_index_struct)) {
						continue;
					}

					str_builder_append(&builder, STR_LIT("        instr_stack_push(out_dependencies, instr->"));
					str_builder_append(&builder, instr_struct_name);
					str_builder_append_char(&builder, '.');
					str_builder_append(&builder, instr_struct->fields[j].name.string);
					str_builder_append(&builder, STR_LIT(");\n"));
				}
				break;
			}
			default:
				unreachable();
			}
		}

		str_builder_append(&builder, STR_LIT("        break;\n"));
	}

	// End of `instr_enumerate_dependencies`
	str_builder_append(&builder, STR_LIT(
				"    case INSTR_COUNT:\n"
				"        unreachable();\n"
				"    }\n"
				"}\n"));

	//
	// instr_print
	//

	size_t max_instr_name_length = 0;

	{
		// Ignore INSTR_COUNT, since it is not a valid instruction kind,
		// and can't be printed.
		size_t variant_count = instr_kind_enum->variant_count - 1;
		for (size_t i = 0; i < variant_count; i += 1) {
			String variant_name = instr_kind_enum->variants[i].name.string;
			size_t name_length = variant_name.length - instr_name_prefix_length;
			max_instr_name_length = max(max_instr_name_length, name_length);
		}
	}

	str_builder_append(&builder, STR_LIT(
				"void instr_print(const Instr* instr) {\n"
				"    String name = instr_name(instr->kind);\n"
				"\n"
				"    size_t name_width = "));
	str_builder_append_int(&builder, max_instr_name_length + 1);
	str_builder_append(&builder, STR_LIT(
				";\n"
				"\n"
				"    printf(\"\\033[32;1m%.*s\\033[0m \\033[%uC\", STR_FMT(name), (uint32_t)(name_width - name.length));\n"
				"\n"
				"    switch (instr->kind) {\n"));

	for (size_t i = 0; i < instr_kind_enum->variant_count - 1; i += 1) {
		String variant_name = instr_kind_enum->variants[i].name.string;
		String instr_struct_name = _instr_kind_to_corresponding_instr_field(instr_kind_enum, i, context->temp_allocator);

		str_builder_append(&builder, STR_LIT("    case "));
		str_builder_append(&builder, variant_name);
		str_builder_append(&builder, STR_LIT(":\n"));

		const ParsedStructField* field = struct_find_field(instr_struct, instr_struct_name);
		if (field) {
			switch (field->type.kind) {
			case PARSED_TYPE_STRUCT:
			case PARSED_TYPE_UNION: {
				const ParsedStruct* instr_struct = field->type.struct_def;
				str_builder_append(&builder, STR_LIT("        printf(\""));

				// First generate the format string
				for (size_t j = 0; j < instr_struct->field_count; j += 1) {
					str_builder_append(&builder, instr_struct->fields[j].name.string);
					str_builder_append(&builder, STR_LIT(": "));

					switch (instr_struct->fields[j].type.kind) {
					case PARSED_TYPE_CHAR:
					case PARSED_TYPE_SIGNED_CHAR:
					case PARSED_TYPE_SHORT:
					case PARSED_TYPE_SIGNED_SHORT:
					case PARSED_TYPE_INT:
					case PARSED_TYPE_SIGNED_INT:
					case PARSED_TYPE_INT8:
					case PARSED_TYPE_INT16:
					case PARSED_TYPE_INT32:
						str_builder_append(&builder, STR_LIT("%d "));
						break;
					case PARSED_TYPE_LONG:
					case PARSED_TYPE_LONG_LONG:
					case PARSED_TYPE_INT64:
					case PARSED_TYPE_SIGNED_LONG:
					case PARSED_TYPE_SIGNED_LONG_LONG:
					case PARSED_TYPE_SIGNED_INT64:
						str_builder_append(&builder, STR_LIT("%lld "));
						break;

					case PARSED_TYPE_UNSIGNED_CHAR:
					case PARSED_TYPE_UNSIGNED_SHORT:
					case PARSED_TYPE_UNSIGNED_INT:
					case PARSED_TYPE_UNSIGNED_INT8:
					case PARSED_TYPE_UNSIGNED_INT16:
					case PARSED_TYPE_UNSIGNED_INT32:
						str_builder_append(&builder, STR_LIT("%u "));
						break;

					case PARSED_TYPE_UNSIGNED_LONG:
					case PARSED_TYPE_UNSIGNED_LONG_LONG:
					case PARSED_TYPE_UNSIGNED_INT64:
						str_builder_append(&builder, STR_LIT("%llu "));
						break;

					case PARSED_TYPE_FLOAT:
					case PARSED_TYPE_DOUBLE:
						str_builder_append(&builder, STR_LIT("%f "));
						break;
					case PARSED_TYPE_ENUM:
						if (type_is_enum(&instr_struct->fields[j].type, instr_bin_op_enum)) {
							str_builder_append(&builder, STR_LIT("%.*s "));
						} else if (type_is_enum(&instr_struct->fields[j].type, compare_kind_enum)) {
							str_builder_append(&builder, STR_LIT("%.*s "));
						}
						break;
					case PARSED_TYPE_STRUCT:
						if (type_is_struct(&instr_struct->fields[j].type, string_struct)) {
							str_builder_append(&builder, STR_LIT("%.*s "));
						} else if (type_is_struct(&instr_struct->fields[j].type, instr_index_struct)) {
							str_builder_append(&builder, STR_LIT("%u "));
						}
						break;
					default:
						unreachable();
					}
				}

				str_builder_append(&builder, STR_LIT("\\n\", "));

				// Now output format arguments
				for (size_t j = 0; j < instr_struct->field_count; j += 1) {
					String pre_arg = {};
					String post_arg = {};

					switch (instr_struct->fields[j].type.kind) {
					case PARSED_TYPE_CHAR:
					case PARSED_TYPE_SIGNED_CHAR:
					case PARSED_TYPE_SHORT:
					case PARSED_TYPE_SIGNED_SHORT:
					case PARSED_TYPE_INT:
					case PARSED_TYPE_SIGNED_INT:
					case PARSED_TYPE_INT8:
					case PARSED_TYPE_INT16:
						pre_arg = STR_LIT("(int32_t)");
						break;
					case PARSED_TYPE_INT32:
						break;
					case PARSED_TYPE_LONG:
					case PARSED_TYPE_LONG_LONG:
					case PARSED_TYPE_INT64:
					case PARSED_TYPE_SIGNED_LONG:
					case PARSED_TYPE_SIGNED_LONG_LONG:
					case PARSED_TYPE_SIGNED_INT64:
						break;

					case PARSED_TYPE_UNSIGNED_CHAR:
					case PARSED_TYPE_UNSIGNED_SHORT:
					case PARSED_TYPE_UNSIGNED_INT:
					case PARSED_TYPE_UNSIGNED_INT8:
					case PARSED_TYPE_UNSIGNED_INT16:
						pre_arg = STR_LIT("(uint32_t)");
						break;
					case PARSED_TYPE_UNSIGNED_INT32:
						break;
					case PARSED_TYPE_UNSIGNED_LONG:
					case PARSED_TYPE_UNSIGNED_LONG_LONG:
					case PARSED_TYPE_UNSIGNED_INT64:
						break;

					case PARSED_TYPE_FLOAT:
					case PARSED_TYPE_DOUBLE:
						break;
					case PARSED_TYPE_ENUM:
						if (type_is_enum(&instr_struct->fields[j].type, instr_bin_op_enum)) {
							pre_arg = STR_LIT("STR_FMT(instr_bin_op_name(");
							post_arg = STR_LIT("))");
						} else if (type_is_enum(&instr_struct->fields[j].type, compare_kind_enum)) {
							pre_arg = STR_LIT("STR_FMT(instr_compare_kind_name(");
							post_arg = STR_LIT("))");
						}
						break;
					case PARSED_TYPE_STRUCT:
						if (type_is_struct(&instr_struct->fields[j].type, string_struct)) {
							pre_arg = STR_LIT("STR_FMT(");
							post_arg = STR_LIT(")");
						} else if (type_is_struct(&instr_struct->fields[j].type, instr_index_struct)) {
							pre_arg = STR_LIT("(uint32_t)");
							post_arg = STR_LIT(".value");
						}
						break;
					default:
						unreachable();
					}

					str_builder_append(&builder, pre_arg);
					str_builder_append(&builder, STR_LIT("instr->"));
					str_builder_append(&builder, field->name.string);
					str_builder_append_char(&builder, '.');
					str_builder_append(&builder, instr_struct->fields[j].name.string);
					str_builder_append(&builder, post_arg);

					if (j != instr_struct->field_count - 1) {
						str_builder_append(&builder, STR_LIT(", "));
					}
				}

				str_builder_append(&builder, STR_LIT(");\n"));
				break;
			}
			default:
				unreachable();
			}
		}

		str_builder_append(&builder, STR_LIT("        break;\n"));
	}

	str_builder_append(&builder, STR_LIT(
				"    case INSTR_COUNT:\n"
				"        unreachable();\n"
				"    }\n"
				"}\n"));

	if (!write_str_to_file("code_gen/src/code_gen/instr.gen.c", builder.string)) {
		return false;
	}

	return true;
}

//
// Main
//

int main() {
	Arena arena = {};
	arena.capacity = align_to_page_size(512 * 8 * 4096);

	Arena diagnostics_arena = {};
	diagnostics_arena.capacity = align_to_page_size(512 * 8 * 4096);

	Arena temp_arena = {};
	temp_arena.capacity = align_to_page_size(512 * 4096);

	Arena generated_tokens_arena = {};
	generated_tokens_arena.capacity = 128 * 4096;

	GeneratorDescriptor generators[] = {
		(GeneratorDescriptor) { .name = STR_LIT("Instr"), .function = _generate_instr },
	};

	for (size_t i = 0; i < array_size(generators); i += 1) {
		ArenaRegion arena_region = arena_begin_temp(&arena);
		ArenaRegion temp_arena_region = arena_begin_temp(&temp_arena);
		ArenaRegion diagnostics_arena_region = arena_begin_temp(&diagnostics_arena);
		ArenaRegion generated_tokens_arena_region = arena_begin_temp(&generated_tokens_arena);

		GenContext context = {};
		context.allocator = &arena;
		context.temp_allocator = &temp_arena;
		context.diagnostics_allocator = &diagnostics_arena;
		context.generated_tokens_allocator = &generated_tokens_arena;

		const GeneratorDescriptor* generator = &generators[i];

		printf("\n  \x1b[1;32mRUN \x1b[0m %.*s\n\n", STR_FMT(generator->name));
		bool result = generator->function(&context);

		if (result) {
			printf("\n  \x1b[1;32mDONE\x1b[0m %.*s\n\n", STR_FMT(generator->name));
		} else {
			printf("\n  \x1b[1;31mFAIL\x1b[0m %.*s\n\n", STR_FMT(generator->name));
		}

		arena_end_temp(arena_region);
		arena_end_temp(temp_arena_region);
		arena_end_temp(diagnostics_arena_region);
		arena_end_temp(generated_tokens_arena_region);
	}

	arena_release(&arena);
	arena_release(&diagnostics_arena);
	arena_release(&temp_arena);

	return EXIT_SUCCESS;
}
