#include "builder/builder_core.h"

int main(int argc, char *argv[]) {
	Arena arena = { .capacity = 16 * 4096 };

	BuildContext context_instance = { .allocator = &arena };
	BuildContext* context = &context_instance;
	
	BuildUnit* project_core = build_begin_project(context, STR_LIT("core"));
	build_add_src_dir(context, STR_LIT("core/src/core/*.c"));
	build_end_project(context);

	BuildUnit* project_parser = build_begin_project(context, STR_LIT("parser"));
	build_add_src_dir(context, STR_LIT("parser/src/parser/*.c"));
	build_add_dependency(context, project_core);
	build_end_project(context);

	BuildUnit* tester = build_begin_project(context, STR_LIT("tester"));
	build_add_src_file(context, STR_LIT("tester/src/tester/tester_main.c"));
	build_add_src_file(context, STR_LIT("tester/src/tester/tests.c"));
	build_end_project(context);

	build_run(context);

	arena_release(&arena);
}
