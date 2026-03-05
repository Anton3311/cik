#include <stdio.h>
#include <stdlib.h>

#include "core/core.h"
#include "parser/preprocessor.h"
#include "parser/parser.h"

int main(int argc, char *argv[]) {
	Arena arena = {};
	arena.capacity = align_to_page_size(512 * 4096);

	Arena diagnostics_arena = {};
	diagnostics_arena .capacity = align_to_page_size(512 * 4096);

	Arena temp_arena = {};
	temp_arena.capacity = align_to_page_size(512 * 4096);

	if (argc == 2) {
		String source_code = read_entire_file_to_str(argv[1], &temp_arena);
		assert(source_code.v != NULL);

		LineInfo line_info = line_info_from_source(&arena, source_code);

		Diagnostics diagnostics = (Diagnostics) {
			.allocator = &diagnostics_arena,
			.source_code = source_code,
			.line_info = line_info,
		};

		Preprocessor preprocessor = {};
		preprocessor_init(&preprocessor,
				str_from_cstr(argv[1]),
				source_code,
				&line_info,
				&diagnostics,
				&arena,
				&temp_arena,
				&arena);

		Parser parser = {};
		parser_init(&parser, &arena, &preprocessor, &diagnostics);

		ParsedAST parsed_ast = {};
		parser_parse(&parser, &parsed_ast);

		if (parsed_ast.root_nodes.first) {
			print_parsed_node(parsed_ast.root_nodes.first);
		}

		diagnostics_print(&diagnostics);
	} else {
		printf("Usage: c <path_to_source_file>\n");
	}

	arena_release(&arena);
	arena_release(&diagnostics_arena);
	arena_release(&temp_arena);

	return EXIT_SUCCESS;
}
