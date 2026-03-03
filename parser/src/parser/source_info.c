#include "source_info.h"

LineInfo line_info_from_source(Arena* allocator, String source) {
	LineInfo line_info = {};
	line_info.line_starts = arena_alloc_array(allocator, uint32_t, 0);
	line_info.line_count = 0;

	size_t line_start = 0;
	size_t read_pos = 0;
	while (read_pos < source.length) {
		if (source.v[read_pos] == '\n') {
			arena_alloc(allocator, uint32_t);
			line_info.line_starts[line_info.line_count] = (uint32_t)line_start;
			line_info.line_count += 1;

			line_start = read_pos + 1;
		}

		read_pos += 1;
	}

	arena_alloc(allocator, uint32_t);
	line_info.line_starts[line_info.line_count] = (uint32_t)line_start;
	line_info.source_length = source.length;

	return line_info;
}

SourceLocation line_info_pos_to_source_location(const LineInfo* line_info, size_t string_pos) {
	assert(string_pos <= line_info->source_length);

	uint32_t left = 0;
	uint32_t right = line_info->line_count + 1;

	while (right - left > 1) {
		uint32_t mid = (right + left) / 2;
		if (line_info->line_starts[mid] > string_pos) {
			right = mid;
		} else {
			left = mid;
		}
	}

	uint32_t line_index = left;
	SourceLocation location = {};
	location.line = line_index;
	location.column = (uint32_t)string_pos - line_info->line_starts[line_index];
	return location;
}

