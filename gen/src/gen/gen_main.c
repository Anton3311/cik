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

//
// Instr Generator
//

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

	IdentifierEntry* instr_struct_entry = ident_storage_find(&ident_storage,
			IDENT_NAMESPACE_TAGGED,
			IDENT_FIND_DEFAULT,
			STR_LIT("Instr"));

	assert(instr_struct_entry);

	const ParsedEnum* instr_kind_enum = ident_storage_find(&ident_storage,
			IDENT_NAMESPACE_ALIAS,
			IDENT_FIND_DEFAULT,
			STR_LIT("InstrKind"))->type_def->aliased_type.enum_def;

	assert(instr_kind_enum);

	StringBuilder builder = { .arena = context->allocator };
	_emit_generated_notice(&builder, __func__);

	_emit_include(&builder, STR_LIT("instr.h"));

	str_builder_append(&builder, STR_LIT("static String instr_kind_to_string[] = {\n"));

	// NOTE: Don't include INSTR_COUNT, which is the last one
	for (size_t i = 0; i < instr_kind_enum->variant_count - 1; i += 1) {
		String variant_name = instr_kind_enum->variants[i].name.string;
		const String prefix = STR_LIT("instr_");
		String simple_name = sub_str(variant_name, prefix.length, variant_name.length - prefix.length);

		str_builder_append(&builder, STR_LIT("    ["));
		str_builder_append(&builder, variant_name);
		str_builder_append(&builder, STR_LIT("] = STR_LIT(\""));
		str_builder_append(&builder, str_to_lower(simple_name, context->temp_allocator));
		str_builder_append(&builder, STR_LIT("\"),\n"));
	}
	str_builder_append(&builder, STR_LIT("};"));

	printf("%.*s\n", STR_FMT(builder.string));

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
