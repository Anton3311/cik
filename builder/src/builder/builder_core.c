#include "builder_core.h"

inline BuildUnit* _get_unit(BuildContext* context, BuildUnitId id) {
	assert((size_t)id.value < context->unit_count);
	return &context->units[id.value];
}

inline BuildUnitId _alloc_unit(BuildContext* context) {
	BuildUnitId unit_id = (BuildUnitId) { (uint16_t)context->unit_count };
	context->unit_count += 1;
	arena_alloc_zeroed(context->allocator, BuildUnit);

	BuildUnit* unit = _get_unit(context, unit_id);
	unit->dependencies = arena_alloc_array(context->dependency_allocator, BuildUnitId, 0);
	return unit_id;
}

void build_init(BuildContext* context, Arena* allocator, Arena* dependency_allocator) {
	context->allocator = allocator;
	context->dependency_allocator = dependency_allocator;

	context->units = arena_alloc_array(context->allocator, BuildUnit, 0);
	context->unit_count = 0;

	context->current_project.value = INVALID_BUILD_UNIT_ID;
}

void build_add_src_dir(BuildContext* context, String dir_path) {
	
}

void build_add_src_file(BuildContext* context, String file_path) {
	assert(context->current_project.value != INVALID_BUILD_UNIT_ID);
	if (!path_exists(context->allocator, file_path)) {
		panic("Given file path doesn't exist");
	}

	BuildUnitId unit_id = _alloc_unit(context);
	BuildUnit* unit = _get_unit(context, unit_id);
	unit->name = path_get_file_name(file_path);
	unit->path = file_path;
	unit->output_type = OUTPUT_OBJ;

	BuildUnit* project = _get_unit(context, context->current_project);
	*arena_alloc(context->dependency_allocator, BuildUnitId) = unit_id;
	project->dependency_count += 1;
}

void build_add_dependency(BuildContext* context, BuildUnitId dependency) {

}

BuildUnitId build_begin_project(BuildContext* context, String name, BuildUnitOutputType output_type) {
	assert(context->current_project.value == INVALID_BUILD_UNIT_ID);
	assert(output_type == OUTPUT_LIB || output_type == OUTPUT_EXE);

	BuildUnitId unit_id = _alloc_unit(context);
	BuildUnit* unit = _get_unit(context, unit_id);
	unit->name = name;
	unit->output_type = output_type;

	context->current_project = unit_id;
	return unit_id;
}

void build_end_project(BuildContext* context) {
	assert(context->current_project.value != INVALID_BUILD_UNIT_ID);

	context->current_project.value = INVALID_BUILD_UNIT_ID;
}

#if 0
void build_output_library(BuildContext* context, String output_dir_path) {
	assert(context->current_project.value != INVALID_BUILD_UNIT_ID);
	
	BuildUnit* unit = _get_unit(context, context->current_project);

	unit->output_type = OUTPUT_LIB;

	StringBuilder path_builder = { .arena = context->allocator };
	str_builder_append(&path_builder, output_dir_path);
	str_builder_append(&path_builder, unit->name);
	str_builder_append(&path_builder, STR_LIT(".lib"));
}
#endif

void build_output_executable(BuildContext* context, String output_dir_path) {
	assert(context->current_project.value != INVALID_BUILD_UNIT_ID);
	
	BuildUnit* unit = _get_unit(context, context->current_project);

	unit->output_type = OUTPUT_EXE;

	StringBuilder path_builder = { .arena = context->allocator };
	str_builder_append(&path_builder, output_dir_path);
	str_builder_append(&path_builder, unit->name);
	str_builder_append(&path_builder, STR_LIT(".exe"));
}

void build_run(BuildContext* context) {
	for (size_t i = 0; i < context->unit_count; i += 1) {
		const BuildUnit* unit = &context->units[i];

		if (unit->output_type == OUTPUT_EXE || unit->output_type == OUTPUT_LIB) {
			printf("build %.*s\n", STR_FMT(unit->name));

			for (size_t i = 0; i < unit->dependency_count; i += 1) {
				printf("\t%.*s\n", STR_FMT(_get_unit(context, unit->dependencies[i])->name));
			}
		}
	}
}
