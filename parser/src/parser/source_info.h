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
} LineInfo;

LineInfo line_info_from_source(Arena* allocator, String source);
SourceLocation line_info_pos_to_source_location(const LineInfo* line_info, size_t string_pos);
String line_info_get_line_string(const LineInfo* line_info, String source_code, size_t line_index);

#endif
