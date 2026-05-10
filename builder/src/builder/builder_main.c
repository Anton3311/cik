#include "builder/builder_core.h"

int main(int argc, char *argv[]) {
	Arena allocator = { .capacity = 16 * 4096 };
	Arena unit_allocator = { .capacity = 16 * 4096 };
	Arena dependency_allocator = { .capacity = 16 * 4096 };

	BuildContext context;
	build_init(&context, &unit_allocator, &dependency_allocator, &allocator);
	
	BuildUnitId core = build_begin_project(&context, STR_LIT("core"), OUTPUT_LIB);
	build_add_src_dir(&context, STR_LIT("core/src/core/"));
	build_add_include(&context, STR_LIT("core/src/"));
	build_end_project(&context);

	BuildUnitId parser = build_begin_project(&context, STR_LIT("parser"), OUTPUT_LIB);
	build_add_src_dir(&context, STR_LIT("parser/src/parser/"));
	build_add_include(&context, STR_LIT("core/src/"));
	build_add_include(&context, STR_LIT("parser/src/"));
	build_add_dependency(&context, core);
	build_end_project(&context);

	BuildUnitId code_gen = build_begin_project(&context, STR_LIT("code_gen"), OUTPUT_LIB);
	build_add_src_dir(&context, STR_LIT("code_gen/src/code_gen"));
	build_add_src_dir(&context, STR_LIT("code_gen/src/code_gen/backends"));
	build_add_include(&context, STR_LIT("code_gen/src"));
	build_add_include(&context, STR_LIT("core/src/"));
	build_add_dependency(&context, core);
	build_end_project(&context);

	BuildUnitId compiler = build_begin_project(&context, STR_LIT("compiler"), OUTPUT_LIB);
	build_add_src_dir(&context, STR_LIT("compiler/src/compiler"));
	build_add_include(&context, STR_LIT("core/src/"));
	build_add_include(&context, STR_LIT("code_gen/src"));
	build_add_include(&context, STR_LIT("parser/src"));
	build_end_project(&context);

	BuildUnitId driver = build_begin_project(&context, STR_LIT("c"), OUTPUT_EXE);
	(void)driver;
	build_add_src_dir(&context, STR_LIT("driver/src/driver"));
	build_add_include(&context, STR_LIT("core/src"));
	build_add_include(&context, STR_LIT("parser/src"));
	build_add_include(&context, STR_LIT("compiler/src"));
	build_add_include(&context, STR_LIT("code_gen/src"));
	build_add_dependency(&context, core);
	build_add_dependency(&context, parser);
	build_add_dependency(&context, compiler);
	build_add_dependency(&context, code_gen);
	build_end_project(&context);

	BuildUnitId generator = build_begin_project(&context, STR_LIT("gen"), OUTPUT_EXE);
	(void)generator;
	build_add_src_dir(&context, STR_LIT("gen/src/gen"));
	build_add_include(&context, STR_LIT("core/src"));
	build_add_include(&context, STR_LIT("parser/src"));
	build_add_dependency(&context, core);
	build_add_dependency(&context, parser);
	build_end_project(&context);

	BuildUnitId tester = build_begin_project(&context, STR_LIT("tester"), OUTPUT_EXE);
	(void)tester;
	build_add_src_file(&context, STR_LIT("tester/src/tester/tester_main.c"));
	build_add_src_file(&context, STR_LIT("tester/src/tester/tests.c"));
	build_add_src_file(&context, STR_LIT("tester/src/tester/tests_x64.c"));
	build_add_include(&context, STR_LIT("core/src/"));
	build_add_include(&context, STR_LIT("tester/src/"));
	build_add_include(&context, STR_LIT("parser/src/"));
	build_add_include(&context, STR_LIT("compiler/src/"));
	build_add_include(&context, STR_LIT("code_gen/src/"));
	build_add_dependency(&context, core);
	build_add_dependency(&context, parser);
	build_add_dependency(&context, compiler);
	build_add_dependency(&context, code_gen);
	build_end_project(&context);

	BuildUnitId test_runner = build_begin_project(&context, STR_LIT("test_runner"), OUTPUT_EXE);
	(void)test_runner;
	build_add_src_file(&context, STR_LIT("tester/src/tester/test_runner.c"));
	build_add_dependency(&context, core);
	build_add_include(&context, STR_LIT("core/src/"));
	build_add_include(&context, STR_LIT("tester/src/"));
	build_end_project(&context);

	build_run(&context, argv, argc);

	arena_release(&allocator);
	arena_release(&unit_allocator);
	arena_release(&dependency_allocator);
}
