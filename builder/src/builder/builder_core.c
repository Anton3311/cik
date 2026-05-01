#include "builder_core.h"

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
	unit->name = path_trim_file_extension(path_get_file_name(file_path));
	unit->path = file_path;
	unit->output_type = OUTPUT_OBJ;

	BuildUnit* project = _get_unit(context, context->current_project);
	*arena_alloc(context->dependency_allocator, BuildUnitId) = unit_id;
	project->dependency_count += 1;
}

void build_add_include(BuildContext* context, String include_dir_path) {
	assert(context->current_project.value != INVALID_BUILD_UNIT_ID);
	if (!path_is_directory(context->unit_allocator, include_dir_path)) {
		panic("Given file path is not a directory");
	}

	BuildUnit* project = _get_unit(context, context->current_project);

	if (project->include_dirs.count == 0) {
		project->include_dirs.values = arena_alloc_array(context->allocator, String, 1);
		project->include_dirs.count = 1;
		project->include_dirs.values[0] = include_dir_path;
	} else {
		*arena_alloc(context->allocator, String) = include_dir_path;
		project->include_dirs.count += 1;
	}
}

void build_add_dependency(BuildContext* context, BuildUnitId dependency) {
	assert(context->current_project.value != INVALID_BUILD_UNIT_ID);

	BuildUnit* project = _get_unit(context, context->current_project);
	*arena_alloc(context->dependency_allocator, BuildUnitId) = dependency;
	project->dependency_count += 1;
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

static void _format_output_file_path(StringBuilder* builder, const BuildUnit* unit) {
	switch (unit->output_type) {
	case OUTPUT_OBJ:
		str_builder_append(builder, STR_LIT("bin/obj/"));
		break;
	case OUTPUT_EXE:
	case OUTPUT_LIB:
		str_builder_append(builder, STR_LIT("bin/"));
		break;
	}

	str_builder_append(builder, unit->name);

	switch (unit->output_type) {
	case OUTPUT_OBJ:
		str_builder_append(builder, STR_LIT(".o"));
		break;
	case OUTPUT_EXE:
		str_builder_append(builder, STR_LIT(".exe"));
		break;
	case OUTPUT_LIB:
		str_builder_append(builder, STR_LIT(".lib"));
		break;
	}
}

static String _generate_source_file_build_cmd(const BuildUnit* unit, StringArray include_dirs, Arena* allocator) {
	StringBuilder builder = { .arena = allocator };

	str_builder_append(&builder, STR_LIT("clang.exe -c "));
	str_builder_append(&builder, unit->path);
	str_builder_append(&builder, STR_LIT(" -m64 -g -Wall -o "));

	_format_output_file_path(&builder, unit);

	for (size_t i = 0; i < include_dirs.count; i += 1) {
		str_builder_append(&builder, STR_LIT(" \"-I"));
		str_builder_append(&builder, include_dirs.values[i]);
		str_builder_append_char(&builder, '\"');
	}

	return builder.string;
}

static String _generate_static_lib_link_cmd(BuildContext* context,
		const BuildUnit* unit,
		Arena* allocator) {
	assert(unit->output_type == OUTPUT_LIB);

	StringBuilder builder = { .arena = allocator };
	str_builder_append(&builder, STR_LIT("llvm-lib.exe "));

	str_builder_append(&builder, STR_LIT("/OUT:"));
	_format_output_file_path(&builder, unit);

	for (size_t i = 0; i < unit->dependency_count; i += 1) {
		str_builder_append_char(&builder, ' ');
		_format_output_file_path(&builder, _get_unit(context, unit->dependencies[i]));
	}

	return builder.string;
}

static String _generate_exe_link_cmd(BuildContext* context, const BuildUnit* unit, Arena* allocator) {
	assert(unit->output_type == OUTPUT_EXE);

	StringBuilder builder = { .arena = allocator };
	str_builder_append(&builder, STR_LIT("clang.exe "));

	str_builder_append(&builder, STR_LIT("-m64 -g -o "));
	_format_output_file_path(&builder, unit);

	for (size_t i = 0; i < unit->dependency_count; i += 1) {
		str_builder_append_char(&builder, ' ');

		BuildUnit* dependency = _get_unit(context, unit->dependencies[i]);
		if (dependency->output_type == OUTPUT_LIB) {
			str_builder_append(&builder, STR_LIT("-l"));
		}

		_format_output_file_path(&builder, dependency);
	}

	str_builder_append(&builder, STR_LIT(" -lDbghelp.lib -lShlwapi.lib -lPathcch.lib -lAdvapi32.lib"));

	return builder.string;
}

void build_run(BuildContext* context) {
	String compiler_exe = STR_LIT("C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\Llvm\\bin\\clang.exe");
	String library_bundler_exe = STR_LIT("C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\Llvm\\bin\\llvm-lib.exe");

	for (size_t i = 0; i < context->unit_count; i += 1) {
		const BuildUnit* unit = &context->units[i];

		if (unit->output_type == OUTPUT_EXE || unit->output_type == OUTPUT_LIB) {
			printf("build %.*s\n", STR_FMT(unit->name));

			for (size_t i = 0; i < unit->dependency_count; i += 1) {
				const BuildUnit* u = _get_unit(context, unit->dependencies[i]);
				printf("\t%.*s\n", STR_FMT(u->name));

				if (u->output_type != OUTPUT_OBJ) {
					continue;
				}

				String cmd = _generate_source_file_build_cmd(u,
						unit->include_dirs,
						context->allocator);

				printf("\t%.*s\n", STR_FMT(cmd));

				int32_t exit_code = 0;
				if (process_run(compiler_exe,
							STR_LIT("."),
							cmd,
							&exit_code,
							context->allocator) == PROCESS_RUN_OK) {
					printf("compiled\n");
				}
			}

			if (unit->output_type == OUTPUT_LIB) {
				String link_cmd = _generate_static_lib_link_cmd(context, unit, context->allocator);
				printf("link: %.*s\n", STR_FMT(link_cmd));

				int32_t exit_code = 0;
				if (process_run(library_bundler_exe,
							STR_LIT("."),
							link_cmd,
							&exit_code,
							context->allocator) == PROCESS_RUN_OK) {
					printf("linked\n");
				}
			} else if (OUTPUT_EXE) {
				String link_cmd = _generate_exe_link_cmd(context, unit, context->allocator);
				printf("link: %.*s\n", STR_FMT(link_cmd));

				int32_t exit_code = 0;
				if (process_run(compiler_exe,
							STR_LIT("."),
							link_cmd,
							&exit_code,
							context->allocator) == PROCESS_RUN_OK) {
					printf("linked\n");
				}
			}
		}
	}
}
