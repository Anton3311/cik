#ifndef BUILDER_CORE_H
#define BUILDER_CORE_H

#include "core/core.h"

typedef struct BuildContext BuildContext;
typedef struct BuildUnit BuildUnit;

typedef enum {
	OUTPUT_NONE,
	OUTPUT_OBJ,
	OUTPUT_LIB,
	OUTPUT_EXE,
} BuildUnitOutputType;

struct BuildUnit {
	String name;
	String path;
	BuildUnitOutputType output_type;

	BuildUnit* dependencies;
	size_t dependecy_count;
};

struct BuildContext {
	Arena* allocator;

	BuildUnit* current_project;
};

void build_add_src_dir(BuildContext* context, String dir_path);
void build_add_src_file(BuildContext* context, String file_path);
void build_add_include(BuildContext* context, String include_dir_path);
void build_add_dependency(BuildContext* context, BuildUnit* dependecy);

BuildUnit* build_begin_project(BuildContext* context, String name);
void build_end_project(BuildContext* context);

void build_output_library(BuildContext* context, String output_dir_path);
void build_output_executable(BuildContext* context, String output_dir_path);

void build_run(BuildContext* context);

#endif
