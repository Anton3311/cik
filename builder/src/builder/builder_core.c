#include "builder_core.h"

#include "Windows.h"

bool _run_process(const char* executable,
		const char* working_dir,
		char* args,
		int32_t* exit_code) {

	PROCESS_INFORMATION process_info = {};
	STARTUPINFOA startup_info = {};

	ZeroMemory(&process_info, sizeof(process_info));
	ZeroMemory(&startup_info, sizeof(startup_info));

	startup_info.cb = sizeof(startup_info);

	BOOL success = CreateProcessA(executable,
			args,
			NULL,
			NULL,
			TRUE,
			0,
			NULL,
			working_dir,
			&startup_info,
			&process_info);

	if (!success) {
		return false;
	}

	WaitForSingleObject(process_info.hProcess, INFINITE);

	if (exit_code) {
		DWORD code = 0;
		GetExitCodeProcess(process_info.hProcess, &code);
		*exit_code = (int32_t)code;
	}

	CloseHandle(process_info.hProcess);
	CloseHandle(process_info.hThread);
	return true;
}

inline BuildUnit* _get_unit(BuildContext* context, BuildUnitId id) {
	assert((size_t)id.value < context->unit_count);
	return &context->units[id.value];
}

inline BuildUnitId _alloc_unit(BuildContext* context) {
	BuildUnitId unit_id = (BuildUnitId) { (uint16_t)context->unit_count };
	context->unit_count += 1;
	arena_alloc_zeroed(context->unit_allocator, BuildUnit);

	BuildUnit* unit = _get_unit(context, unit_id);
	unit->dependencies = arena_alloc_array(context->dependency_allocator, BuildUnitId, 0);
	return unit_id;
}

void build_init(BuildContext* context, Arena* unit_allocator, Arena* dependency_allocator, Arena* allocator) {
	context->allocator = allocator;
	context->unit_allocator = unit_allocator;
	context->dependency_allocator = dependency_allocator;

	context->units = arena_alloc_array(context->unit_allocator, BuildUnit, 0);
	context->unit_count = 0;

	context->current_project.value = INVALID_BUILD_UNIT_ID;
}

void build_add_src_dir(BuildContext* context, String dir_path) {
	assert(context->current_project.value != INVALID_BUILD_UNIT_ID);
	if (!path_exists(context->unit_allocator, dir_path)) {
		panic("Given file path doesn't exist");
	}

	StringArray file_paths = fs_enumerate_files_in_directory(dir_path,
			context->allocator,
			context->unit_allocator);

	for (size_t i = 0; i < file_paths.count; i += 1) {
		String file_path = file_paths.values[i];
		String extension = path_get_file_extension(file_path);
		if (str_equal(extension, STR_LIT(".c"))) {
			build_add_src_file(context, path_append(dir_path, file_path, context->allocator));
		}
	}
}

void build_add_src_file(BuildContext* context, String file_path) {
	assert(context->current_project.value != INVALID_BUILD_UNIT_ID);
	if (!path_exists(context->unit_allocator, file_path)) {
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

	StringBuilder path_builder = { .arena = context->unit_allocator};
	str_builder_append(&path_builder, output_dir_path);
	str_builder_append(&path_builder, unit->name);
	str_builder_append(&path_builder, STR_LIT(".lib"));
}
#endif

void build_output_executable(BuildContext* context, String output_dir_path) {
	assert(context->current_project.value != INVALID_BUILD_UNIT_ID);
	
	BuildUnit* unit = _get_unit(context, context->current_project);

	unit->output_type = OUTPUT_EXE;

	StringBuilder path_builder = { .arena = context->unit_allocator};
	str_builder_append(&path_builder, output_dir_path);
	str_builder_append(&path_builder, unit->name);
	str_builder_append(&path_builder, STR_LIT(".exe"));
}

static char* _generate_source_file_build_cmd(const BuildUnit* unit, Arena* allocator) {
	StringBuilder builder = { .arena = allocator };

	str_builder_append(&builder, STR_LIT("clang.exe -c "));
	str_builder_append(&builder, unit->path);
	str_builder_append(&builder, STR_LIT(" -m64 -g -Wall -o "));
	str_builder_append(&builder, STR_LIT("bin\\obj\\"));
	str_builder_append(&builder, path_trim_file_extension(unit->path));
	str_builder_append(&builder, STR_LIT(".o"));
	return str_builder_to_cstr(&builder);
}

void build_run(BuildContext* context) {
	for (size_t i = 0; i < context->unit_count; i += 1) {
		const BuildUnit* unit = &context->units[i];

		if (unit->output_type == OUTPUT_EXE || unit->output_type == OUTPUT_LIB) {
			printf("build %.*s\n", STR_FMT(unit->name));

			for (size_t i = 0; i < unit->dependency_count; i += 1) {
				const BuildUnit* u = _get_unit(context, unit->dependencies[i]);
				printf("\t%.*s\n", STR_FMT(u->name));

				char* cmd = _generate_source_file_build_cmd(u, context->allocator);
				printf("\t%s\n", cmd);

				int32_t exit_code = 0;
				_run_process("C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\Llvm\\bin\\clang.exe", ".", cmd, &exit_code);
			}
		}
	}
}
