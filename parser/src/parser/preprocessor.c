#include "preprocessor.h"

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
	line_info.line_starts[line_info.line_count] = (uint32_t)source.length;

	return line_info;
}

SourceLocation line_info_pos_to_source_location(const LineInfo* line_info, size_t string_pos) {
	if (string_pos >= line_info->line_starts[line_info->line_count]) {
		return (SourceLocation) {};
	}

	uint32_t left = 0;
	uint32_t right = line_info->line_count;

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

String line_info_get_line_string(const LineInfo* line_info, String source_code, size_t line_index) {
	uint32_t line_length = line_info->line_starts[line_index + 1] - line_info->line_starts[line_index];
	return sub_str(source_code, line_info->line_starts[line_index], line_length - 1);
}

//
// Preprocessor
//

void preprocessor_init(Preprocessor* state, String source_code, Arena* allocator) {
	state->line_info = line_info_from_source(allocator, source_code);
	state->tokenizer = (Tokenizer) {
		.source_code = source_code,
		.read_position = 0,
	};
}

void preprocessor_skip_derective(Preprocessor* state) {
	Token next_token = tokenizer_next_token(&state->tokenizer);
	assert(next_token.kind == TOKEN_IDENT);

	if (str_equal(next_token.string, STR_LIT("include"))) {
		tokenizer_skip_whitespace_and_comments(&state->tokenizer);

		if (_tokenizer_has_next_char(&state->tokenizer, '<')) {
			Token string_token = {};
			StringTokenizerResult result = _tokenizer_try_create_string_token(&state->tokenizer, '<', '>', &string_token);
			assert(result == STR_TOKEN_RESULT_NONE);
		} else if (_tokenizer_has_next_char(&state->tokenizer, '"')) {
			Token string_token = {};
			StringTokenizerResult result = _tokenizer_try_create_string_token(&state->tokenizer, '"', '"', &string_token);
			assert(result == STR_TOKEN_RESULT_NONE);
		}

		// TODO: Use parsed include statements
	} else {
		SourceLocation start_location = line_info_pos_to_source_location(&state->line_info, next_token.source_range.start);
		SourceLocation end_location = line_info_pos_to_source_location(&state->line_info, next_token.source_range.end);

		for (uint32_t line = start_location.line; line <= end_location.line; line += 1) {
			String line_string = line_info_get_line_string(&state->line_info, state->tokenizer.source_code, line);
			printf("%.*s\n", STR_FMT(line_string));
		}

		unreachable_msg("Unhandled preprocessor derective type");
	}
}

