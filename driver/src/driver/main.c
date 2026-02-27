#include <stdio.h>
#include <stdlib.h>

#include "core/core.h"
#include "parser/preprocessor.h"

#define HELLo

int main(int argc, char *argv[]) {
	Arena arena = {};
	arena.capacity = align_to_page_size(512 * 4096);

	Arena temp_arena = {};
	temp_arena.capacity = align_to_page_size(512 * 4096);

	if (argc == 2) {
		String source_code = read_entire_file_to_str(argv[1], &temp_arena);
		assert(source_code.v != NULL);

		Preprocessor preprocessor = {};
		preprocessor_init(&preprocessor, source_code, &temp_arena);

		Diagnostics diagnostics = (Diagnostics) {
			.allocator = &arena,
			.source_code = source_code,
			.line_info = preprocessor.line_info,
		};

		preprocessor.diagnostics = &diagnostics;

		while (true) {
			Token token = preprocessor_next_token(&preprocessor);
			String source_range = sub_str(source_code,
					token.source_range.start,
					token.source_range.end - token.source_range.start);

			assert(str_equal(token.string, source_range));

			printf("%.*s\n", STR_FMT(token.string));
			if (token.kind == TOKEN_EOF) {
				break;
			}
		}

		diagnostics_print(&diagnostics);
	} else {
		printf("Usage: c <path_to_source_file>\n");
	}

	arena_release(&arena);
	arena_release(&temp_arena);

	return EXIT_SUCCESS;
}
