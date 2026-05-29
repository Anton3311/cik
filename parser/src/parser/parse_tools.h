#ifndef PARSE_TOOLS_H
#define PARSE_TOOLS_H

#include <stdint.h>

#include "parser/diagnostics.h"

typedef enum {
	INT_LIT_FMT_DECIMAL,
	INT_LIT_FMT_BIN,
	INT_LIT_FMT_OCTAL,
	INT_LIT_FMT_HEX,
} IntergerLiteralFormat;

String int_literal_format_to_string(IntergerLiteralFormat format);

typedef enum {
	INT_SUFIX_NONE,
	INT_SUFIX_U,
	INT_SUFIX_L,
	INT_SUFIX_UL,
	INT_SUFIX_LL,
	INT_SUFIX_ULL,
} IntegerLiteralSufixKind;

typedef struct {
	IntergerLiteralFormat format;
	SourceString int_part_string;

	bool has_sufix;
	uint8_t sufix_bit_count; // 8, 16, 32 or 64
	IntegerLiteralSufixKind sufix_kind;
} IntegerLiteralInfo;

typedef struct {
	uint64_t value;

	IntergerLiteralFormat format;

	bool has_sufix;
	uint8_t sufix_bit_count;
	IntegerLiteralSufixKind sufix_kind;
} IntLiteral;

bool parse_int_literal(Token token, Diagnostics* diagnostics, IntLiteral* out_result);

#endif
