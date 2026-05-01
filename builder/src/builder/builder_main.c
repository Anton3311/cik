#include "builder/builder_core.h"

int main(int argc, char *argv[]) {
	Arena allocator = { .capacity = 16 * 4096 };
	Arena dependency_allocator = { .capacity = 16 * 4096 };

	BuildContext context;
	build_init(&context, &allocator, &dependency_allocator);
	
	BuildUnitId project_core = build_begin_project(&context, STR_LIT("core"), OUTPUT_LIB);
	build_add_src_dir(&context, STR_LIT("core/src/core/*.c"));
	build_end_project(&context);

	BuildUnitId project_parser = build_begin_project(&context, STR_LIT("parser"), OUTPUT_LIB);
	build_add_src_dir(&context, STR_LIT("parser/src/parser/*.c"));
	build_add_dependency(&context, project_core);
	build_end_project(&context);

	BuildUnitId tester = build_begin_project(&context, STR_LIT("tester"), OUTPUT_EXE);
	build_add_src_file(&context, STR_LIT("tester/src/tester/tester_main.c"));
	build_add_src_file(&context, STR_LIT("tester/src/tester/tests.c"));
	build_end_project(&context);

	build_run(&context);

	arena_release(&allocator);
	arena_release(&dependency_allocator);
}
