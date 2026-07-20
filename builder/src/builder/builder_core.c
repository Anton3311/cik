#include "builder_core.h"

typedef struct {
	String clang_path;
	String llvm_lib_path;
} ClangCompilerInfo;

typedef struct {
	String cl_path;
	String lib_path;
	String link_path;
} MsvcCompilerInfo;

typedef enum {
	COMPILER_KIND_CLANG,
	COMPILER_KIND_MSVC,
} CompilerKind;

static ClangCompilerInfo s_clang_info;
static MsvcCompilerInfo s_msvc_compiler_path;
static CompilerKind s_current_compiler;

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

	BuildUnit* project = _get_unit(context, context->current_project);

	BuildUnitId unit_id = _alloc_unit(context);
	BuildUnit* unit = _get_unit(context, unit_id);
	unit->name = path_trim_file_extension(path_get_file_name(file_path));
	unit->path = file_path;
	unit->output_type = OUTPUT_OBJ;
	unit->compile_options = project->compile_options;

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
	unit->compile_options = context->default_compile_options;

	context->current_project = unit_id;
	return unit_id;
}

void build_end_project(BuildContext* context) {
	assert(context->current_project.value != INVALID_BUILD_UNIT_ID);

	context->current_project.value = INVALID_BUILD_UNIT_ID;
}

void build_set_compiler_options(BuildContext* context,
		BuildUnitId unit_id,
		FileBuildOptions options) {
	BuildUnit* project = _get_unit(context, context->current_project);
	BuildUnit* unit = _get_unit(context, unit_id);
	unit->compile_options = options | project->compile_options;
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

static void _format_output_file_path(StringBuilder* builder,
		const BuildUnit* unit,
		bool include_file_ext) {
	switch (unit->output_type) {
	case OUTPUT_OBJ:
		str_builder_append(builder, STR_LIT("bin/obj/"));
		break;
	case OUTPUT_EXE:
	case OUTPUT_LIB:
		str_builder_append(builder, STR_LIT("bin/"));
		break;
	case OUTPUT_NONE:
		unreachable();
	}

	char* formated_name = (char*)builder->string.v + builder->string.length;
	str_builder_append(builder, unit->name);

	// Repalce '.' with '_'
	for (size_t i = 0; i < unit->name.length; i += 1) {
		if (formated_name[i] == '.') {
			formated_name[i] = '_';
		}
	}
	
	if (!include_file_ext) {
		return;
	}

	switch (unit->output_type) {
	case OUTPUT_OBJ:
		switch (s_current_compiler) {
		case COMPILER_KIND_CLANG:
			str_builder_append(builder, STR_LIT(".o"));
			break;
		case COMPILER_KIND_MSVC:
			str_builder_append(builder, STR_LIT(".obj"));
			break;
		}
		break;
	case OUTPUT_EXE:
		str_builder_append(builder, STR_LIT(".exe"));
		break;
	case OUTPUT_LIB:
		str_builder_append(builder, STR_LIT(".lib"));
		break;
	case OUTPUT_NONE:
		unreachable();
	}
}

void _clang_gen_file_compile_cmd(BuildContext* context,
		StringBuilder* cmd_builder,
		const BuildUnit* unit) {

	str_builder_append(cmd_builder, STR_LIT("-c "));
	str_builder_append(cmd_builder, unit->path);
	str_builder_append_char(cmd_builder, ' ');

	switch (context->target_arch) {
	case ARCH_X64:
		str_builder_append(cmd_builder, STR_LIT("-m64 "));
		break;
	}

	if (!str_equal(path_get_file_extension(unit->path), STR_LIT(".cpp"))) {
		switch (context->language) {
		case LANG_C99:
			str_builder_append(cmd_builder, STR_LIT("-std=c99 "));
			break;
		case LANG_C11:
			str_builder_append(cmd_builder, STR_LIT("-std=c11 "));
			break;
		}
	}

	FileBuildOptions options = unit->compile_options;
	if (has_flag(options, COMPILE_OPTION_GENERATE_DEBUG_INFO)) {
		str_builder_append(cmd_builder, STR_LIT("-g "));
	}

	if (has_flag(options, COMPILE_OPTION_WARNINGS_ALL)) {
		str_builder_append(cmd_builder, STR_LIT("-Wall "));
	}

	if (has_flag(options, COMPILE_OPTION_OPTIMIZE)) {
		str_builder_append(cmd_builder, STR_LIT("-O3 "));
	}

	str_builder_append(cmd_builder, STR_LIT("-o "));
	_format_output_file_path(cmd_builder, unit, true);
	str_builder_append_char(cmd_builder, ' ');

	StringArray include_dirs = unit->include_dirs;
	for (size_t i = 0; i < include_dirs.count; i += 1) {
		str_builder_append(cmd_builder, STR_LIT("\"-I"));
		str_builder_append(cmd_builder, include_dirs.values[i]);
		str_builder_append(cmd_builder, STR_LIT("\" "));
	}

	for (size_t i = 0; i < context->define_count; i += 1) {
		MacroDefine def = context->defines[i];
		str_builder_append(cmd_builder, STR_LIT("\"-D"));
		str_builder_append(cmd_builder, def.name);

		if (def.value.length > 0) {
			str_builder_append_char(cmd_builder, '=');
			str_builder_append(cmd_builder, def.value);
		}

		str_builder_append(cmd_builder, STR_LIT("\" "));
	}
}

static void _clang_gen_static_lib_link_cmd(BuildContext* context,
		StringBuilder* builder,
		const BuildUnit* unit) {
	assert(unit->output_type == OUTPUT_LIB);

	str_builder_append(builder, STR_LIT("/OUT:"));
	_format_output_file_path(builder, unit, true);

	for (size_t i = 0; i < unit->dependency_count; i += 1) {
		str_builder_append_char(builder, ' ');
		_format_output_file_path(builder, _get_unit(context, unit->dependencies[i]), true);
	}
}

static void _clang_gen_exe_link_cmd(BuildContext* context,
		StringBuilder* builder,
		const BuildUnit* unit) {
	assert(unit->output_type == OUTPUT_EXE);

	str_builder_append(builder, STR_LIT("-m64 -g -o "));
	_format_output_file_path(builder, unit, true);

	for (size_t i = 0; i < unit->dependency_count; i += 1) {
		str_builder_append_char(builder, ' ');

		BuildUnit* dependency = _get_unit(context, unit->dependencies[i]);
		if (dependency->output_type == OUTPUT_LIB) {
			str_builder_append(builder, STR_LIT("-l"));
		}

		_format_output_file_path(builder, dependency, true);
	}

	str_builder_append(builder, STR_LIT(" -lDbghelp.lib -lShlwapi.lib -lPathcch.lib -lAdvapi32.lib"));
}

static void _msvc_gen_file_compile_cmd(BuildContext* context,
		StringBuilder* cmd_builder,
		const BuildUnit* unit) {

	str_builder_append(cmd_builder, STR_LIT("/nologo /c "));
	str_builder_append(cmd_builder, unit->path);
	str_builder_append_char(cmd_builder, ' ');

	switch (context->target_arch) {
	case ARCH_X64:
		break;
	}

	switch (context->language) {
	case LANG_C99:
		break;
	case LANG_C11:
		str_builder_append(cmd_builder, STR_LIT("/std:c11 "));
		break;
	}

	FileBuildOptions options = unit->compile_options;
	if (has_flag(options, COMPILE_OPTION_GENERATE_DEBUG_INFO)) {
		str_builder_append(cmd_builder, STR_LIT("/Z7 "));
	}

	if (has_flag(options, COMPILE_OPTION_WARNINGS_ALL)) {
		str_builder_append(cmd_builder, STR_LIT("/Wall "));
	}

	if (has_flag(options, COMPILE_OPTION_SANITIZE_ADDRESS)) {
		str_builder_append(cmd_builder, STR_LIT("/fsanitize=address "));
	}

	if (has_flag(options, COMPILE_OPTION_OPTIMIZE)) {
		str_builder_append(cmd_builder, STR_LIT("/O2 "));
	}

	str_builder_append(cmd_builder, STR_LIT("/Fo: "));
	_format_output_file_path(cmd_builder, unit, false);
	str_builder_append_char(cmd_builder, ' ');

	StringArray include_dirs = unit->include_dirs;
	for (size_t i = 0; i < include_dirs.count; i += 1) {
		str_builder_append(cmd_builder, STR_LIT("\"/I"));
		str_builder_append(cmd_builder, include_dirs.values[i]);
		str_builder_append(cmd_builder, STR_LIT("\" "));
	}

	for (size_t i = 0; i < context->define_count; i += 1) {
		MacroDefine def = context->defines[i];
		str_builder_append(cmd_builder, STR_LIT("\"/D"));
		str_builder_append(cmd_builder, def.name);

		if (def.value.length > 0) {
			str_builder_append_char(cmd_builder, '=');
			str_builder_append(cmd_builder, def.value);
		}

		str_builder_append(cmd_builder, STR_LIT("\" "));
	}
}

static void _msvc_gen_static_lib_link_cmd(BuildContext* context,
		StringBuilder* builder,
		const BuildUnit* unit) {

	str_builder_append(builder, STR_LIT("/nologo /OUT:"));
	_format_output_file_path(builder, unit, true);

	for (size_t i = 0; i < unit->dependency_count; i += 1) {
		str_builder_append_char(builder, ' ');
		_format_output_file_path(builder, _get_unit(context, unit->dependencies[i]), true);
	}
}

static void _msvc_gen_exe_link_cmd(BuildContext* context,
		StringBuilder* builder,
		const BuildUnit* unit) {
	assert(unit->output_type == OUTPUT_EXE);

	// TODO: For debug configs link against LIBCMTD.lib, for release LIBCMT.lib
	str_builder_append(builder, STR_LIT("/nologo /incremental:no /opt:ref /opt:icf"));

	for (size_t i = 0; i < unit->dependency_count; i += 1) {
		str_builder_append_char(builder, ' ');

		BuildUnit* dependency = _get_unit(context, unit->dependencies[i]);
		_format_output_file_path(builder, dependency, true);
		str_builder_append_char(builder, ' ');
	}

	str_builder_append(builder, STR_LIT("/OUT:"));
	_format_output_file_path(builder, unit, true);
	str_builder_append_char(builder, ' ');

	str_builder_append(builder, STR_LIT("/DEBUG LIBCMTD.lib "));

	str_builder_append(builder, STR_LIT("Dbghelp.lib Shlwapi.lib Pathcch.lib Advapi32.lib"));
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

// `project_name` specify the name of the project the dependencies will be enumerated.
// If it is empty, the queue will contains dependencies for all the projects
static BuildQueue _build_build_queue(BuildContext* context, String project_name) {
	BuildUnitId* root_units = arena_alloc_array(context->allocator, BuildUnitId, 0);
	size_t root_unit_count = 0;

	// Collect root units
	for (size_t i = 0; i < context->unit_count; i += 1) {
		const BuildUnit* unit = &context->units[i];

		bool matches = false;
		if (project_name.length > 0) {
			matches = (unit->output_type == OUTPUT_EXE || unit->output_type == OUTPUT_LIB)
				&& str_equal(unit->name, project_name);
		} else {
			matches = unit->output_type == OUTPUT_EXE;
		}

		if (!matches) {
			continue;
		}

		BuildUnitId* id = arena_alloc(context->allocator, BuildUnitId);
		id->value = (uint16_t)i;
		root_unit_count += 1;
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

typedef struct {
	String cmd_log;
} BuildResult;

static bool _run_build_process(BuildContext* context,
		String project_name,
		bool cmd_log_enabled,
		BuildResult* out_result) {

	BuildQueue build_queue = _build_build_queue(context, project_name);

	if (build_queue.count == 0) {
		fprintf(stderr, "\033[31;1mNo targets to build\n\033[0m");
		return false;
	}

	bool result = true;

	// Build units
	UnitStatus* status = arena_alloc_array_zeroed(context->allocator,
			UnitStatus,
			context->unit_count);

	StringBuilder cmd_log = { .arena = context->unit_allocator };
	for (size_t i = 0; i < build_queue.count; i += 1) {
		ArenaRegion temp = arena_begin_temp(context->allocator);

		BuildUnitId unit_id = build_queue.units[i];
		BuildUnit* unit = _get_unit(context, unit_id);
		if (!_verify_dependecies_status(context, unit_id, status)) {
			_print_result(false, unit->name, STR_LIT("skipped due to failed dependencies"));
			status[unit_id.value] = UNIT_STATUS_FAILED;
			continue;
		}

		String step_name = {};
		String exe_path = {};

		StringBuilder cmd_builder = { .arena = context->allocator };

		switch (unit->output_type) {
		case OUTPUT_EXE: {
			step_name = STR_LIT("link");

			switch (s_current_compiler) {
			case COMPILER_KIND_CLANG:
				exe_path = s_clang_info.clang_path;

				str_builder_append(&cmd_builder, path_get_file_name(exe_path));
				str_builder_append_char(&cmd_builder, ' ');
				_clang_gen_exe_link_cmd(context, &cmd_builder, unit);
				break;
			case COMPILER_KIND_MSVC:
				exe_path = s_msvc_compiler_path.link_path;

				str_builder_append(&cmd_builder, path_get_file_name(exe_path));
				str_builder_append_char(&cmd_builder, ' ');
				_msvc_gen_exe_link_cmd(context, &cmd_builder, unit);
				break;
			}
			break;
		}
		case OUTPUT_LIB: {
			step_name = STR_LIT("link");

			switch (s_current_compiler) {
			case COMPILER_KIND_CLANG:
				exe_path = s_clang_info.llvm_lib_path;

				str_builder_append(&cmd_builder, path_get_file_name(exe_path));
				str_builder_append_char(&cmd_builder, ' ');
				_clang_gen_static_lib_link_cmd(context, &cmd_builder, unit);
				break;
			case COMPILER_KIND_MSVC:
				exe_path = s_msvc_compiler_path.lib_path;

				str_builder_append(&cmd_builder, path_get_file_name(exe_path));
				str_builder_append_char(&cmd_builder, ' ');
				_msvc_gen_static_lib_link_cmd(context, &cmd_builder, unit);
				break;
			}

			break;
		}
		case OUTPUT_OBJ: {
			step_name = STR_LIT("compile");

			switch (s_current_compiler) {
			case COMPILER_KIND_CLANG:
				exe_path = s_clang_info.clang_path;

				str_builder_append(&cmd_builder, path_get_file_name(exe_path));
				str_builder_append_char(&cmd_builder, ' ');
				_clang_gen_file_compile_cmd(context, &cmd_builder, unit);
				break;
			case COMPILER_KIND_MSVC:
				exe_path = s_msvc_compiler_path.cl_path;

				str_builder_append(&cmd_builder, path_get_file_name(exe_path));
				str_builder_append_char(&cmd_builder, ' ');
				_msvc_gen_file_compile_cmd(context, &cmd_builder, unit);
				break;
			}
			break;
		}
		case OUTPUT_NONE:
			unreachable();
		}

		if (cmd_log_enabled) {
			str_builder_append(&cmd_log, cmd_builder.string);
			str_builder_append_char(&cmd_log, '\n');
			status[unit_id.value] = UNIT_STATUS_DONE;
		}

		int32_t exit_code = 0;
		bool success = process_run(exe_path,
					STR_LIT("."),
					cmd_builder.string,
					&exit_code,
					context->allocator) == PROCESS_RUN_OK;

		if (success && exit_code == 0) {
			status[unit_id.value] = UNIT_STATUS_DONE;
		} else {
			status[unit_id.value] = UNIT_STATUS_FAILED;
			result = false;
		}

		_print_result(status[unit_id.value] == UNIT_STATUS_DONE, step_name, unit->name);

		arena_end_temp(temp);
	}

	if (cmd_log_enabled) {
		out_result->cmd_log = cmd_log.string;
	}

	return result;
}

static void _print_all_targets(const BuildContext* context) {
	printf("\nAvailable targets:\n\n");
	for (size_t i = 0; i < context->unit_count; i += 1) {
		const BuildUnit* unit = &context->units[i];

		const char* output_type_string;
		switch (unit->output_type) {
		case OUTPUT_LIB:
			output_type_string = "lib";
			break;
		case OUTPUT_EXE:
			output_type_string = "exe";
			break;
		default:
			continue;
		}

		printf("  %s \033[1;32m%.*s\033[0m\n", output_type_string, STR_FMT(unit->name));
	}

	printf("\n");
}

static bool _try_resolve_clang_exes(String dir,
		ClangCompilerInfo* out_info,
		Arena* allocator) {
	ArenaRegion temp = arena_begin_temp(allocator);

	bool result = false;

	String clang_path = path_append(dir, STR_LIT("clang.exe"), allocator);
	String llvm_lib_path = path_append(dir, STR_LIT("llvm-lib.exe"), allocator);

	bool clang_exists = path_exists(allocator, clang_path);
	bool llvm_lib_exists = path_exists(allocator, llvm_lib_path);
	result = clang_exists && llvm_lib_exists;

	if (result) {
		out_info->clang_path = clang_path;
		out_info->llvm_lib_path = llvm_lib_path;
	} else {
		// Undo allocations
		arena_end_temp(temp);
	}

	return result;
}

static bool _try_resolve_msvc_exes(String dir,
		MsvcCompilerInfo* out_info,
		Arena* allocator) {
	ArenaRegion temp = arena_begin_temp(allocator);

	bool result = false;

	String cl_path = path_append(dir, STR_LIT("cl.exe"), allocator);
	String link_path = path_append(dir, STR_LIT("link.exe"), allocator);
	String lib_path = path_append(dir, STR_LIT("lib.exe"), allocator);

	bool cl_exists = path_exists(allocator, cl_path);
	bool link_exists = path_exists(allocator, link_path);
	bool lib_exists = path_exists(allocator, lib_path);

	result = cl_exists && link_exists && lib_exists;

	if (result) {
		out_info->cl_path = cl_path;
		out_info->link_path = link_path;
		out_info->lib_path = lib_path;
	} else {
		// Undo allocations
		arena_end_temp(temp);
	}

	return result;
}

static bool _find_compiler_in_path(String paths,
		Arena* allocator,
		Arena* temp_allocator) {

	ArenaRegion temp = arena_begin_temp(temp_allocator);

	bool result = false;
	while (paths.length) {
		String path = paths;
		for (size_t i = 0; i < paths.length; i += 1) {
			if (paths.v[i] == ';') {
				path = sub_str(paths, 0, i);
				paths.v += i + 1;
				paths.length -= i + 1;
				break;
			}
		}

		switch (s_current_compiler) {
		case COMPILER_KIND_MSVC:
			result = _try_resolve_msvc_exes(path, &s_msvc_compiler_path, allocator);
			break;
		case COMPILER_KIND_CLANG:
			result = _try_resolve_clang_exes(path, &s_clang_info, allocator);
			break;
		}

		if (result) {
			break;
		}

		if (str_equal(path, paths)) {
			break;
		}
	}

	arena_end_temp(temp);
	return result;
}

static const char* s_help_message =
	"\n"
	"  Usage:\n"
	"    bb.exe <command> [<args>]\n"
	"\n"
	"  Commands:\n"
	"    build                 build all projects and generate scripts/build_all.bat\n"
	"                          use --compiler-paths=<compiler-search-paths> to specify a ';'\n"
	"                          separated list of search paths.\n"
	"                          By default `PATH` environment variable is used\n"
	"\n"
	"    build <project-name>  build specific project\n"
	"\n"
	"    list                  show a list of available build targets\n"
	"\n"
	"    help                  show help message\n";

int32_t build_run(BuildContext* context) {
	BuildResult result = {};
	int32_t exit_code = _run_build_process(context,
			context->target_project_name,
			context->command_log_enabled,
			&result)
		? EXIT_SUCCESS
		: EXIT_FAILURE;

	if (context->command_log_enabled) {
		const char* build_all_path = "scripts/build_all.bat";
		bool written = write_str_to_file(build_all_path, result.cmd_log);
		_print_result(written, str_from_cstr(build_all_path), (String) {});

		if (!written) {
			exit_code = EXIT_FAILURE;
		}
	}

	return exit_code;
}

void build_init(BuildContext* context,
		const char** argv,
		size_t argc,
		Arena* unit_allocator,
		Arena* dependency_allocator,
		Arena* allocator) {
	context->allocator = allocator;
	context->unit_allocator = unit_allocator;
	context->dependency_allocator = dependency_allocator;

	context->units = arena_alloc_array(context->unit_allocator, BuildUnit, 0);
	context->unit_count = 0;

	context->current_project.value = INVALID_BUILD_UNIT_ID;

	if (argc <= 1) {
		printf("\033[1;31mNot enough arguments\033[0m\n");
		printf("%s", s_help_message);
		exit(EXIT_FAILURE);
	}

	s_current_compiler = COMPILER_KIND_CLANG;
	s_clang_info = (ClangCompilerInfo) {};
	s_msvc_compiler_path = (MsvcCompilerInfo) {};

	size_t arg_index = 1;
	while (arg_index < argc) {
		if (strcmp(argv[arg_index], "build") == 0) {
			arg_index += 1;

			if (arg_index < argc) {
				String arg = str_from_cstr(argv[arg_index]);
				if (!str_starts_with(arg, STR_LIT("--"))) {
					context->target_project_name = str_from_cstr(argv[arg_index]);
					arg_index += 1;
				}
			}

			if (context->target_project_name.length == 0) {
				context->command_log_enabled = true;
			}

			String compiler_paths_prefix = STR_LIT("--compiler-paths=");
			String compiler_kind_prefix = STR_LIT("--cc=");

			String compiler_search_paths = {};
			while (arg_index < argc) {
				String arg = str_from_cstr(argv[arg_index]);
				if (str_starts_with(arg, compiler_paths_prefix)) {
					compiler_search_paths = sub_str(arg,
							compiler_paths_prefix.length,
							arg.length - compiler_paths_prefix.length);

					if (compiler_search_paths.length == 0) {
						fprintf(stderr, "Empty --compiler-paths\n");
						exit(EXIT_FAILURE);
					}

					arg_index += 1;
				} else if (str_starts_with(arg, compiler_kind_prefix)) {
					String compiler_name = sub_str(arg,
							compiler_kind_prefix.length,
							arg.length - compiler_kind_prefix.length);

					if (str_equal(compiler_name, STR_LIT("clang"))) {
						s_current_compiler = COMPILER_KIND_CLANG;
					} else if (str_equal(compiler_name, STR_LIT("cl"))) {
						s_current_compiler = COMPILER_KIND_MSVC;
					} else {
						fprintf(stderr, "Unknown compiler '%.*s'\n", STR_FMT(compiler_name));
						exit(EXIT_FAILURE);
					}

					arg_index += 1;
				} else {
					bool resolved = false;
					for (size_t i = 0; i < context->custom_flag_def_count; i += 1) {
						if (str_equal(context->custom_flag_defs[i].string, arg)) {
							arg_index += 1;
							context->custom_flags |= context->custom_flag_defs[i].value;
							resolved = true;
							break;
						}
					}

					if (!resolved) {
						fprintf(stderr, "Unknown build argument: %.*s\n", STR_FMT(arg));
						exit(EXIT_FAILURE);
					}
				}
			}

			if (compiler_search_paths.length == 0) {
				compiler_search_paths = env_get("PATH", context->allocator);
			}

			if (!_find_compiler_in_path(compiler_search_paths,
						context->allocator,
						context->unit_allocator)) {
				fprintf(stderr, "Compiler not found in the search path");
				exit(EXIT_FAILURE);
			}
		} else if (strcmp(argv[arg_index], "help") == 0) {
			printf("%s", s_help_message);
			exit(EXIT_SUCCESS);
		} else if (strcmp(argv[arg_index], "list") == 0) {
			_print_all_targets(context);
			exit(EXIT_SUCCESS);
		} else {
			fprintf(stderr, "\033[1;31mUnknown argument: '%s'\033[0m", argv[arg_index]);
			exit(EXIT_FAILURE);
		}
	}
}
