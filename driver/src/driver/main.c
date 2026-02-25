#include <stdio.h>
#include <stdlib.h>

#include "core/core.h"
#include "parser/parser.h"

int main(int argc, char *argv[]) {
	Arena arena = {};
	arena.capacity = align_to_page_size(16 * 4096);

	Arena temp_arena = {};
	temp_arena.capacity = align_to_page_size(16 * 4096);

	if (argc == 2) {
		String source_code = read_entire_file_to_str(argv[1], &temp_arena);
		assert(source_code.v != NULL);

		Tokenizer tokenizer = (Tokenizer) {
			.source_code = source_code,
			.read_position = 0,
		};

		while (true) {
			Token token = tokenizer_next_token(&tokenizer);
			printf("%.*s\n", STR_FMT(token.string));
			if (token.kind == TOKEN_EOF) {
				break;
			}
		}
	} else {
		printf("Usage: c <path_to_source_file>\n");
	}

	arena_release(&arena);
	arena_release(&temp_arena);

	return EXIT_SUCCESS;
}
