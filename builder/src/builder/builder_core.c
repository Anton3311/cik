#include "builder_core.h"

void build_add_src_dir(BuildContext* context, String dir_path) {

}

void build_add_src_file(BuildContext* context, String file_path) {

}

void build_add_dependency(BuildContext* context, BuildUnit* dependecy) {

}

BuildUnit* build_begin_project(BuildContext* context, String name) {
	assert(context->current_project == NULL);

	BuildUnit* unit = arena_alloc(context->allocator, BuildUnit);
	unit->name = name;

	context->current_project = unit;
	return unit;
}

void build_end_project(BuildContext* context) {
	assert(context->current_project != NULL);

	context->current_project = NULL;
}

void build_output_library(BuildContext* context, String output_dir_path) {
	assert(context->current_project != NULL);
	
	BuildUnit* unit = context->current_project;

	unit->output_type = OUTPUT_LIB;

	StringBuilder path_builder = { .arena = context->allocator };
	str_builder_append(&path_builder, output_dir_path);
	str_builder_append(&path_builder, unit->name);
	str_builder_append(&path_builder, STR_LIT(".lib"));
}

void build_output_executable(BuildContext* context, String output_dir_path) {
	assert(context->current_project != NULL);
	
	BuildUnit* unit = context->current_project;

	unit->output_type = OUTPUT_EXE;

	StringBuilder path_builder = { .arena = context->allocator };
	str_builder_append(&path_builder, output_dir_path);
	str_builder_append(&path_builder, unit->name);
	str_builder_append(&path_builder, STR_LIT(".exe"));
}

void build_run(BuildContext* context) {

}
