#include "diagnostics.h"

void _print_indent(size_t count) {
	for (size_t i = 0; i < count; i += 1) {
		printf("\t");
	}
}

void _diagnostics_print_entry(const Diagnostics* diagnostics, const DiagnosticsEntry* entry, size_t indent) {
	while (entry != NULL) {
		// TODO: Handle multiple highlight ranges
		assert(entry->highlighted_range_count == 1);

		const SourceFile* source_file = entry->source_file;
		assert(source_file);
		const LineInfo* line_info = &source_file->line_info;

		uint32_t start_line = entry->start_line;
		if (start_line > 0) {
			start_line -= 1;
		}

		size_t highlight_index = 0;

		printf("\n");

		uint32_t end_line = min(entry->end_line + 1, line_info->line_count - 1);
		for (uint32_t line = start_line; line <= end_line; line += 1) {
			String source_line = line_info_get_line_string(line_info, source_file->source_code, line);

			SourceRange line_range = line_info_get_line_range(line_info, line);
			SourceRange highlight_range = entry->highlighted_ranges[highlight_index];

			size_t highlight_start = min(max(highlight_range.start, line_range.start), line_range.end);
			size_t highlight_end = min(max(highlight_range.end, line_range.start), line_range.end);

			// Normalize to range [0; source_line.length]
			highlight_start -= line_range.start;
			highlight_end -= line_range.start;

			String before_highlight = sub_str(source_line, 0, highlight_start);
			String highlighted_sub_str = sub_str(source_line, highlight_start, highlight_end - highlight_start);
			String after_highlight = sub_str(source_line, highlight_end, source_line.length - highlight_end);

			_print_indent(indent);
			printf("\t%u: %.*s\033[1;31m%.*s\033[0m%.*s\n",
					line + 1,
					STR_FMT(before_highlight),
					STR_FMT(highlighted_sub_str),
					STR_FMT(after_highlight));
		}

		_print_indent(indent);
		printf("%.*s\n", STR_FMT(entry->message));

		if (entry->first_child) {
			_diagnostics_print_entry(diagnostics, entry->first_child, indent + 1);
		}

		entry = entry->next;
	}
}

void diagnostics_print(const Diagnostics* diagnostics) {
	DiagnosticsEntry* entry = diagnostics->first;
	_diagnostics_print_entry(diagnostics, entry, 0);
}

DiagnosticsEntry* diagnostics_report_error(Diagnostics* diagnostics,
		SourceRange source_range,
		String message,
		DiagnosticsEntry* parent) {
	assert(source_range.source_file);
	const LineInfo* line_info = &source_range.source_file->line_info;

	DiagnosticsEntry* entry = arena_alloc(diagnostics->allocator, DiagnosticsEntry);
	entry->start_line = line_info_pos_to_source_location(line_info, source_range.start).line;
	entry->end_line = line_info_pos_to_source_location(line_info, source_range.end).line;
	entry->source_file = source_range.source_file;

	entry->highlighted_ranges = arena_alloc(diagnostics->allocator, SourceRange);
	entry->highlighted_ranges[0] = source_range;
	entry->highlighted_range_count = 1;

	entry->message = message;

	if (parent == NULL) {
		entry->next = NULL;

		if (diagnostics->first == NULL) {
			diagnostics->first = entry;
			diagnostics->last = entry;
		} else {
			assert(diagnostics->last);
			diagnostics->last->next = entry;
		}

		diagnostics->last = entry;
	} else {
		entry->next = NULL;

		if (parent->first_child == NULL) {
			parent->first_child = entry;
			parent->last_child = entry;
		} else {
			assert(parent->last_child);
			parent->last_child->next = entry;
		}
	}

	return entry;
}

void diagnostics_report_unexpected_token(Diagnostics* diagnostics,
		Token actual_token,
		TokenKind* expected_kinds,
		size_t expected_kind_count) {
	String actual_token_string = actual_token.string;
	if (actual_token.kind == TOKEN_EOF) {
		actual_token_string = token_kind_to_string(actual_token.kind);
	}

	StringBuilder builder = { .arena = diagnostics->allocator };
	str_builder_append(&builder, STR_LIT("Unexpected token '"));
	str_builder_append(&builder, actual_token_string);

	str_builder_append(&builder, STR_LIT("'. Expected: "));
	if (expected_kind_count == 1) {
		str_builder_append(&builder, token_kind_to_string(expected_kinds[0]));
	} else if (expected_kind_count == 2) {
		str_builder_append(&builder, token_kind_to_string(expected_kinds[0]));
		str_builder_append(&builder, STR_LIT(" or "));
		str_builder_append(&builder, token_kind_to_string(expected_kinds[1]));
	} else if (expected_kind_count > 2) {
		for (size_t i = 0; i < expected_kind_count - 2; i += 1) {
			str_builder_append(&builder, token_kind_to_string(expected_kinds[i]));
			str_builder_append(&builder, STR_LIT(", "));
		}

		str_builder_append(&builder, token_kind_to_string(expected_kinds[expected_kind_count - 2]));
		str_builder_append(&builder, STR_LIT(" or "));
		str_builder_append(&builder, token_kind_to_string(expected_kinds[expected_kind_count - 1]));
	}

	diagnostics_report_error(diagnostics, actual_token.source_range, builder.string, NULL);
}

