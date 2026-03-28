#ifndef DIAGNOSTICS_H
#define DIAGNOSTICS_H

#include "core/core.h"
#include "parser/source_info.h"
#include "parser/tokenizer.h"

typedef enum {
	DIAGNOSTICS_ENTRY_ERROR,
} DiagnosticsEntryKind;

typedef struct DiagnosticsEntry DiagnosticsEntry;

struct DiagnosticsEntry {
	const SourceFile* source_file;

	uint32_t start_line;
	uint32_t end_line;
	SourceRange* highlighted_ranges;
	size_t highlighted_range_count;
	DiagnosticsEntryKind kind;
	String message;

	DiagnosticsEntry* first_child;
	DiagnosticsEntry* last_child;

	DiagnosticsEntry* next;
};

typedef struct {
	Arena* allocator;

	DiagnosticsEntry* first;
	DiagnosticsEntry* last;
} Diagnostics;

void diagnostics_print(const Diagnostics* diagnostics);

DiagnosticsEntry* diagnostics_report_error(Diagnostics* diagnostics,
		SourceRange source_range,
		String message,
		DiagnosticsEntry* parent);

void diagnostics_report_unexpected_token(Diagnostics* diagnostics,
		Token actual_token,
		TokenKind* expected_kinds,
		size_t expected_kind_count);

#endif
