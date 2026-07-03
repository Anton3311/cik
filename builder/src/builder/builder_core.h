#ifndef BUILDER_CORE_H
#define BUILDER_CORE_H

#include "core/core.h"

typedef struct BuildContext BuildContext;
typedef struct BuildUnit BuildUnit;
typedef struct BuildUnitId BuildUnitId;

typedef enum {
	OUTPUT_NONE,
	OUTPUT_OBJ,
	OUTPUT_LIB,
	OUTPUT_EXE,
} BuildUnitOutputType;

#define INVALID_BUILD_UNIT_ID UINT16_MAX

typedef enum {
	ARCH_X64
} TargetArch;

typedef enum {
	LANG_C99,
	LANG_C11,
} Language;

typedef enum {
	COMPILE_OPTION_NONE                = 0,
	COMPILE_OPTION_GENERATE_DEBUG_INFO = 1 << 0,
	COMPILE_OPTION_WARNINGS_ALL        = 1 << 1,
	COMPILE_OPTION_SANITIZE_ADDRESS    = 1 << 2,
} FileBuildOptions;

struct BuildUnitId {
	uint16_t value;
};

struct BuildUnit {
	String name;
	String path;
	BuildUnitOutputType output_type;

	FileBuildOptions compile_options;

	BuildUnitId* dependencies;
	size_t dependency_count;

	StringArray include_dirs;
};

struct BuildContext {
	Arena* unit_allocator;
	Arena* allocator;
	Arena* dependency_allocator;

	TargetArch target_arch;
	Language language;

	FileBuildOptions default_compile_options;

	BuildUnitId current_project;

	BuildUnit* units;
	size_t unit_count;
};

void build_init(BuildContext* context, Arena* unit_allocator, Arena* dependency_allocator, Arena* allocator);

void build_add_src_dir(BuildContext* context, String dir_path);
void build_add_src_file(BuildContext* context, String file_path);
void build_add_include(BuildContext* context, String include_dir_path);
void build_add_dependency(BuildContext* context, BuildUnitId dependecy);

BuildUnitId build_begin_project(BuildContext* context, String name, BuildUnitOutputType output_type);
void build_end_project(BuildContext* context);

void build_output_library(BuildContext* context, String output_dir_path);
void build_output_executable(BuildContext* context, String output_dir_path);

void build_set_compiler_options(BuildContext* context,
		BuildUnitId unit_id,
		FileBuildOptions options);

int32_t build_run(BuildContext* context, char* argv[], size_t argc);

#endif
