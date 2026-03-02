#ifndef SOURCE_INFO_H
#define SOURCE_INFO_H

#include "core/core.h"

//
// LineInfo
//

typedef struct {
	size_t start;
	size_t end;
} SourceRange;

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
String line_info_get_line_string(const LineInfo* line_info, String source_code, size_t line_index);

inline SourceRange source_range_from_sub_string(String source_code, String sub_string) {
	assert(sub_string.v >= source_code.v);
	assert(sub_string.v + sub_string.length <= source_code.v + source_code.length);

	size_t range_start = (size_t)(sub_string.v - source_code.v);
	return (SourceRange) {
		.start = range_start,
		.end = range_start + sub_string.length,
	};
}

#endif
