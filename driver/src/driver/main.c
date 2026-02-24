#include <stdio.h>
#include <stdlib.h>

#include "core/core.h"

int main(int argc, char *argv[]) {
	Arena arena = {};
	arena.capacity = align_to_page_size(16 * 4096);

	arena_release(&arena);

	return EXIT_SUCCESS;
}
