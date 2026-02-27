#ifndef DIAGNOSTICS_H
#define DIAGNOSTICS_H

#include "core/core.h"
#include "parser/source_info.h"

typedef enum {
	DIAGNOSTICS_ENTRY_ERROR,
} DiagnosticsEntryKind;

typedef struct DiagnosticsEntry DiagnosticsEntry;

struct DiagnosticsEntry {
	uint32_t start_line;
	uint32_t end_line;
	SourceRange* highlighted_ranges;
	size_t highlighted_range_count;
	DiagnosticsEntryKind kind;
	String message;

	DiagnosticsEntry* prev;
	DiagnosticsEntry* next;
};

typedef struct {
	Arena* allocator;

	String source_code;
	LineInfo line_info;

	DiagnosticsEntry* first;
	DiagnosticsEntry* last;
} Diagnostics;

void diagnostics_print(const Diagnostics* diagnostics);
void diagnostics_report_error(Diagnostics* diagnostics, SourceRange source_range, String message);

#endif
