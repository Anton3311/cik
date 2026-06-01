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

struct BuildUnitId {
	uint16_t value;
};

struct BuildUnit {
	String name;
	String path;
	BuildUnitOutputType output_type;

	BuildUnitId* dependencies;
	size_t dependency_count;

	StringArray include_dirs;
};

struct BuildContext {
	Arena* unit_allocator;
	Arena* allocator;
	Arena* dependency_allocator;

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

int32_t build_run(BuildContext* context, char* argv[], size_t argc);

#endif
