#include "parser/tokenizer.h"

// Run command (from `test_self_hosted_tokenizer` in `tester/tests_self_hosting.c`):
// bin\\c.exe .\\tester\\src\\tester\\self_hosting\\self_hosted_tokenizer_driver.c -Istdx -Icore/src/ -Iparser/src .\\parser\\src\\parser\\tokenizer.c --no-report-exit-code -- c.exe <file_path>

// NOTE: `core.c` is not compiled together with this file, since it includes `windows.h` which the
//       parser is not yet capable of fully parsing.
//       
//       Some of the functions are implemented in the header (`core.h`), other in `core.c`. Here,
//       we provide stub implementation of such functions, that are only implemented in `core.c`.

size_t path_get_file_name_start(String path) { return 0; }

bool is_debugger_connected() {
	return false;
}

bool try_print_stack_trace(size_t skipped_frame_count) {
	return true;
}

void print_assertion_stack_trace() {}
void _arena_reserve(Arena*) {}
void _arena_commit(Arena*, size_t) {}

String file_read_all_text(const char* path) {
	FILE* file = fopen(path, "rb");
	if (file == NULL) {
		return (String) {};
	}

	fseek(file, 0, SEEK_END);
	size_t size = ftell(file);
	fseek(file, 0, SEEK_SET);

	char* buffer = malloc(size);
	fread(buffer, 1, size, file);

	fclose(file);
	return (String) { .v = buffer, .length = size };
}

int main(int argc, char* argv[]) {
	if (argc < 2) {
		printf("expected a file path\n");
		return EXIT_FAILURE;
	}

	String source = file_read_all_text(argv[1]);
	if (source.v == NULL) {
		printf("failed to read the file\n");
		return EXIT_FAILURE;
	}

	Tokenizer tokenizer = { .source_code = source };
	
	while (true) {
		Token token = tokenizer_next_token(&tokenizer);
		if (token.kind == TOKEN_EOF) {
			break;
		}

		printf("%u %zu %zu %.*s\n",
				token.kind,
				token.source_range.start,
				token.source_range.end,
				STR_FMT(token.string));
	}

	free((char*)source.v);

	return 0;
}
