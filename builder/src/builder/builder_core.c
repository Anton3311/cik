#include "builder_core.h"

static String COMPILER_EXE =
	STR_LIT("C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\Llvm\\bin\\clang.exe");
static String LIBRARY_BUNDLER_EXE =
	STR_LIT("C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\Llvm\\bin\\llvm-lib.exe");

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

typedef enum {
	UNIT_STATUS_NONE,
	UNIT_STATUS_DONE,
	UNIT_STATUS_FAILED,
} UnitStatus;

typedef struct {
	BuildUnitId* units;
	size_t count;
	size_t capacity;
} BuildQueue;

static void _build_queue_init(BuildQueue* queue, Arena* allocator, size_t capacity) {
	queue->units = arena_alloc_array(allocator, BuildUnitId, capacity);
	queue->count = 0;
	queue->capacity = capacity;
}

static void _build_queue_append(BuildQueue* queue, BuildUnitId unit_id) {
	assert(queue->count < queue->capacity);
	queue->units[queue->count] = unit_id;
	queue->count += 1;
}

static void _append_unit_dependecies(BuildContext* context, BuildQueue* queue, BuildUnitId unit_id, BitArray* visited_units) {
	assert(!bit_array_get(visited_units, unit_id.value));

	bit_array_set(visited_units, unit_id.value, true);

	BuildUnit* unit = _get_unit(context, unit_id);
	for (size_t i = 0; i < unit->dependency_count; i += 1) {
		BuildUnitId dependency_id = unit->dependencies[i];
		if (bit_array_get(visited_units, dependency_id.value)) {
			continue;
		}

		// HACK: Need a better way of passing include dirs from
		//       the project down to each source file
		BuildUnit* dependency = _get_unit(context, dependency_id);
		dependency->include_dirs = unit->include_dirs;

		_append_unit_dependecies(context, queue, dependency_id, visited_units);
	}

	_build_queue_append(queue, unit_id);
}

static bool _verify_dependecies_status(BuildContext* context, BuildUnitId unit_id, const UnitStatus* unit_status) {
	BuildUnit* unit = _get_unit(context, unit_id);
	for (size_t i = 0; i < unit->dependency_count; i += 1) {
		BuildUnitId dependency = unit->dependencies[i];
		if (unit_status[dependency.value] != UNIT_STATUS_DONE) {
			return false;
		}
	}

	return true;
}

static BuildQueue _build_build_queue(BuildContext* context) {
	BuildUnitId* root_units = arena_alloc_array(context->allocator, BuildUnitId, 0);
	size_t root_unit_count = 0;

	// Collect root units
	for (size_t i = 0; i < context->unit_count; i += 1) {
		const BuildUnit* unit = &context->units[i];
		if (unit->output_type == OUTPUT_EXE) {
			BuildUnitId* id = arena_alloc(context->allocator, BuildUnitId);
			id->value = (uint16_t)i;
			root_unit_count += 1;
		}
	}

	// Create build queue
	BuildQueue build_queue;
	_build_queue_init(&build_queue, context->allocator, context->unit_count);
	BitArray visited_units = bit_array_alloc(context->allocator, context->unit_count);
	bit_array_clear(&visited_units);

	for (size_t i = 0; i < root_unit_count; i += 1) {
		_append_unit_dependecies(context, &build_queue, root_units[i], &visited_units);
	}

	return build_queue;
}

static void _print_result(bool success, String operation_name, String message) {
	if (success) {
		printf("  \x1b[1;32mDONE\x1b[0m %.*s %.*s\n", STR_FMT(operation_name), STR_FMT(message));
	} else {
		printf("  \x1b[1;31mFAIL\x1b[0m %.*s %.*s\n", STR_FMT(operation_name), STR_FMT(message));
	}
}

static void _run_build_process(BuildContext* context) {
	BuildQueue build_queue = _build_build_queue(context);

	// Build units
	UnitStatus* status = arena_alloc_zeroed_array(context->allocator, UnitStatus, context->unit_count);
	for (size_t i = 0; i < build_queue.count; i += 1) {
		ArenaRegion temp = arena_begin_temp(context->allocator);

		BuildUnitId unit_id = build_queue.units[i];
		BuildUnit* unit = _get_unit(context, unit_id);
		if (!_verify_dependecies_status(context, unit_id, status)) {
			_print_result(false, unit->name, STR_LIT("skipped due to failed dependencies"));
			status[unit_id.value] = UNIT_STATUS_FAILED;
			continue;
		}

		switch (unit->output_type) {
		case OUTPUT_EXE: {
			String link_cmd = _generate_exe_link_cmd(context, unit, context->allocator);

			int32_t exit_code = 0;
			bool success = process_run(COMPILER_EXE,
						STR_LIT("."),
						link_cmd,
						&exit_code,
						context->allocator) == PROCESS_RUN_OK;
			if (success && exit_code == 0) {
				status[unit_id.value] = UNIT_STATUS_DONE;
			} else {
				status[unit_id.value] = UNIT_STATUS_FAILED;
			}

			_print_result(status[unit_id.value] == UNIT_STATUS_DONE, STR_LIT("link"), unit->name);
			break;
		}
		case OUTPUT_LIB: {
			String link_cmd = _generate_static_lib_link_cmd(context, unit, context->allocator);

			int32_t exit_code = 0;
			bool success = process_run(LIBRARY_BUNDLER_EXE,
						STR_LIT("."),
						link_cmd,
						&exit_code,
						context->allocator) == PROCESS_RUN_OK;
			if (success && exit_code == 0) {
				status[unit_id.value] = UNIT_STATUS_DONE;
			} else {
				status[unit_id.value] = UNIT_STATUS_FAILED;
			}

			_print_result(status[unit_id.value] == UNIT_STATUS_DONE, STR_LIT("link"), unit->name);
			break;
		}
		case OUTPUT_OBJ: {
			String cmd = _generate_source_file_build_cmd(unit,
					unit->include_dirs,
					context->allocator);

			int32_t exit_code = 0;
			bool success = process_run(COMPILER_EXE,
						STR_LIT("."),
						cmd,
						&exit_code,
						context->allocator) == PROCESS_RUN_OK;
			if (success && exit_code == 0) {
				status[unit_id.value] = UNIT_STATUS_DONE;
			} else {
				status[unit_id.value] = UNIT_STATUS_FAILED;
			}

			_print_result(status[unit_id.value] == UNIT_STATUS_DONE, STR_LIT("compile"), unit->path);
			break;
		}
		case OUTPUT_NONE:
			break;
		}

		arena_end_temp(temp);
	}
}

typedef enum {
	CMD_BUILD,
	CMD_INVALID,
} Command;

void build_run(BuildContext* context, const char** argv, size_t argc) {
	bool has_error = false;
	Command command = CMD_INVALID;
	if (argc == 1) {
		return;
	} else if (argc > 1) {
		if (strcmp(argv[1], "build") == 0) {
			command = CMD_BUILD;
		} else {
			fprintf(stderr, "Unknown command: '%s'", argv[1]);
			has_error = true;
		}

		for (size_t i = 2; i < argc; i += 1) {
			fprintf(stderr, "Unknown argument: '%s'", argv[i]);
			has_error = true;
		}

		if (has_error) {
			return;
		}
	}

	switch (command) {
	case CMD_BUILD:
		_run_build_process(context);
		break;
	}
}
