#include "diagnostics.h"

void diagnostics_print(const Diagnostics* diagnostics) {
	DiagnosticsEntry* entry = diagnostics->first;
	while (entry != NULL) {

		uint32_t start_line = entry->start_line;
		if (start_line > 0) {
			start_line -= 1;
		}

		uint32_t end_line = min(entry->end_line + 1, diagnostics->line_info.line_count - 1);
		for (uint32_t line = start_line; line <= end_line; line += 1) {
			String source_line = line_info_get_line_string(&diagnostics->line_info, diagnostics->source_code, line);
			printf("\t%u: %.*s\n", line, STR_FMT(source_line));
		}

		printf("%.*s\n\n", STR_FMT(entry->message));

		entry = entry->next;
	}
}

void diagnostics_report_error(Diagnostics* diagnostics, SourceRange source_range, String message) {
	DiagnosticsEntry* entry = arena_alloc(diagnostics->allocator, DiagnosticsEntry);
	entry->start_line = line_info_pos_to_source_location(&diagnostics->line_info, source_range.start).line;
	entry->end_line = line_info_pos_to_source_location(&diagnostics->line_info, source_range.end).line;

	entry->highlighted_ranges = arena_alloc(diagnostics->allocator, SourceRange);
	entry->highlighted_ranges[0] = source_range;
	entry->highlighted_range_count = 1;

	entry->message = message;

	entry->prev = diagnostics->last;
	entry->next = NULL;

	diagnostics->last = entry;
	if (diagnostics->first == NULL) {
		diagnostics->first = entry;
	}
}
