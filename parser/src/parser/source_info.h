#ifndef SOURCE_INFO_H
#define SOURCE_INFO_H

#include "core/core.h"

typedef struct SourceFile SourceFile;

typedef struct {
	String string;
	const SourceFile* source_file;
} SourceString;

typedef struct {
	const SourceFile* source_file;
	size_t start;
	size_t end;
} SourceRange;

//
// LineInfo
//

typedef struct {
	uint32_t line;
	uint32_t column;
} SourceLocation;

typedef struct {
	uint32_t line_count;
	uint32_t* line_starts;
	size_t source_length;
} LineInfo;

LineInfo line_info_from_source(Arena* allocator, String source);
SourceLocation line_info_pos_to_source_location(const LineInfo* line_info, size_t string_pos);

inline SourceRange line_info_get_line_range(const LineInfo* line_info, size_t line_index) {
	assert(line_index < (size_t)line_info->line_count);
	return (SourceRange) {
		.start = line_info->line_starts[line_index],
		.end = line_info->line_starts[line_index + 1],
	};
}

inline String line_info_get_line_string(const LineInfo* line_info, String source_code, size_t line_index) {
	SourceRange line_range = line_info_get_line_range(line_info, line_index);
	size_t line_length = line_range.end - line_range.start;
	return sub_str(source_code, line_range.start, line_length);
}

inline SourceRange source_range_from_sub_string(String source_code, String sub_string) {
	assert(sub_string.v >= source_code.v);
	assert(sub_string.v + sub_string.length <= source_code.v + source_code.length);

	size_t range_start = (size_t)(sub_string.v - source_code.v);
	return (SourceRange) {
		.start = range_start,
		.end = range_start + sub_string.length,
	};
}

//
// SourceStorage
//

typedef struct {
	size_t value;
} SourceFileId;

struct SourceFile {
	String path;
	String source_code;
	LineInfo line_info;
};

typedef struct {
	StringArray include_dirs;
	SourceFile* files;
	size_t count;
	size_t capacity;

	Arena* allocator;
} SourceStorage;

void source_storage_init(SourceStorage* storage, StringArray include_dirs, Arena* allocator);

String source_storage_resolve_include_path(const SourceStorage* storage,
		String include_path,
		const SourceFile* current_file,
		Arena* allocator,
		Arena* temp_allocator);

SourceFile* source_storage_append(SourceStorage* storage, String path, String source_code);
SourceFile* source_storage_append_from_path(SourceStorage* storage, String path, Arena* temp_allocator);
SourceFile* source_storage_find_file(SourceStorage* storage, String path); 

inline SourceRange source_string_to_range(SourceString string) {
	SourceRange range = source_range_from_sub_string(string.source_file->source_code, string.string);
	range.source_file = string.source_file;
	return range;
}

#endif
