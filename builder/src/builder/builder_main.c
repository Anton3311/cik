#include "builder/builder_core.h"

int main(int argc, char *argv[]) {
	Arena allocator = { .capacity = 16 * 4096 };
	Arena unit_allocator = { .capacity = 16 * 4096 };
	Arena dependency_allocator = { .capacity = 16 * 4096 };

	BuildContext context;
	build_init(&context, &unit_allocator, &dependency_allocator, &allocator);
	
	BuildUnitId project_core = build_begin_project(&context, STR_LIT("core"), OUTPUT_LIB);
	build_add_src_dir(&context, STR_LIT("core/src/core/"));
	build_add_include(&context, STR_LIT("core/src/"));
	build_end_project(&context);

	BuildUnitId project_parser = build_begin_project(&context, STR_LIT("parser"), OUTPUT_LIB);
	build_add_src_dir(&context, STR_LIT("parser/src/parser/"));
	build_add_include(&context, STR_LIT("core/src/"));
	build_add_include(&context, STR_LIT("parser/src/"));
	build_add_dependency(&context, project_core);
	build_end_project(&context);

	BuildUnitId driver = build_begin_project(&context, STR_LIT("c"), OUTPUT_EXE);
	build_add_src_dir(&context, STR_LIT("driver/src/driver"));
	build_add_include(&context, STR_LIT("core/src"));
	build_add_include(&context, STR_LIT("parser/src"));
	build_add_dependency(&context, project_core);
	build_add_dependency(&context, project_parser);
	build_end_project(&context);

	BuildUnitId tester = build_begin_project(&context, STR_LIT("tester"), OUTPUT_EXE);
	build_add_src_file(&context, STR_LIT("tester/src/tester/tester_main.c"));
	build_add_src_file(&context, STR_LIT("tester/src/tester/tests.c"));
	build_add_dependency(&context, project_core);
	build_add_dependency(&context, project_parser);
	build_add_include(&context, STR_LIT("core/src/"));
	build_add_include(&context, STR_LIT("tester/src/"));
	build_add_include(&context, STR_LIT("parser/src/"));
	build_end_project(&context);

	BuildUnitId test_runner = build_begin_project(&context, STR_LIT("test_runner"), OUTPUT_EXE);
	build_add_src_file(&context, STR_LIT("tester/src/tester/test_runner.c"));
	build_add_dependency(&context, project_core);
	build_add_include(&context, STR_LIT("core/src/"));
	build_add_include(&context, STR_LIT("tester/src/"));
	build_end_project(&context);

	build_run(&context);

	arena_release(&allocator);
	arena_release(&dependency_allocator);
}
