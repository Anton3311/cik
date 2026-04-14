#include <stdio.h>
#include <stdlib.h>

#include "core/core.h"
#include "parser/preprocessor.h"
#include "parser/parser.h"

static bool enum_installed_win_sdks(String sdk_install_path,
		Arena* allocator,
		Arena* temp_allocator,
		StringArray* out_sdks) {

	*out_sdks = fs_enumerate_entries_in_directory(sdk_install_path, FS_ENTRY_DIRECTORY, allocator, temp_allocator);
	return true;
}

int main(int argc, char *argv[]) {
	Arena arena = {};
	arena.capacity = align_to_page_size(512 * 4096);

	Arena diagnostics_arena = {};
	diagnostics_arena.capacity = align_to_page_size(512 * 4096);

	Arena temp_arena = {};
	temp_arena.capacity = align_to_page_size(512 * 4096);

	String install_path = {};
	if (!win_sdk_get_install_path(&arena, &install_path)) {
		fprintf(stderr, "Failed to read Windows SDK install path");
		return EXIT_FAILURE;
	}

	install_path = path_append(install_path, STR_LIT("Include"), &arena);

	StringArray sdks = {};
	if (!enum_installed_win_sdks(install_path, &arena, &temp_arena, &sdks)) {
		fprintf(stderr, "Failed to detect Windows SDK");
		return EXIT_FAILURE;
	}

	if (argc >= 2) {
		String source_code = read_entire_file_to_str(argv[1], &temp_arena);
		assert(source_code.v != NULL);

		SourceStorage source_storage = {};

		String sdk_path = path_append(install_path, sdks.values[0], &arena);
		String um_include_path = path_append(sdk_path, STR_LIT("um"), &arena);
		String ucrt_include_path = path_append(sdk_path, STR_LIT("ucrt"), &arena);
		String shared_include_path = path_append(sdk_path, STR_LIT("shared"), &arena);

		StringArray include_dirs = {};
		include_dirs.values = arena_alloc_array(&arena, String, 0);

		str_array_append(&include_dirs, &arena, um_include_path);
		str_array_append(&include_dirs, &arena, ucrt_include_path);
		str_array_append(&include_dirs, &arena, shared_include_path);

		for (size_t i = 2; i < (size_t)argc; i += 1) {
			String arg = str_from_cstr(argv[i]);
			if (arg.length >= 2 && arg.v[0] == '-' && arg.v[1] == 'I') {
				String include_path = sub_str(arg, 2, arg.length - 2);
				assert(include_path.length > 0);
				str_array_append(&include_dirs, &arena, include_path);
			} else {
				fprintf(stderr, "Unknown argument '%s'", argv[i]);
				return EXIT_FAILURE;
			}
		}

		source_storage_init(&source_storage,
				include_dirs,
				&arena);

		SourceFile* source_file = source_storage_append_from_path(&source_storage, str_from_cstr(argv[1]), &temp_arena);

		Diagnostics diagnostics = (Diagnostics) {
			.allocator = &diagnostics_arena,
		};

		Arena generated_tokens_arena = {};
		generated_tokens_arena.capacity = 128 * 4096;

		Preprocessor preprocessor = {};
		preprocessor_init(&preprocessor,
				&source_storage,
				source_file,
				&diagnostics,
				&arena,
				&temp_arena,
				&generated_tokens_arena);

		Arena ident_arena = { .capacity = 128 * 4096 };
		Arena ast_arena = { .capacity = 128 * 4096 };

		IdentifierStorage ident_storage = {};
		ident_storage_init(&ident_storage, &ident_arena);

		Parser parser = {};
		parser_init(&parser, &ast_arena, &ident_storage, &preprocessor, &diagnostics);

		ParsedAST parsed_ast = {};
		parser_parse(&parser, &parsed_ast);

		if (parsed_ast.root_nodes.first) {
			print_parsed_node(parsed_ast.root_nodes.first);
		}

		ident_storage_release(&ident_storage);

		arena_release(&ident_arena);
		arena_release(&ast_arena);
		arena_release(&generated_tokens_arena);

		diagnostics_print(&diagnostics);
	} else {
		printf("Usage: c <path_to_source_file>\n");
	}

	arena_release(&arena);
	arena_release(&diagnostics_arena);
	arena_release(&temp_arena);

	return EXIT_SUCCESS;
}
