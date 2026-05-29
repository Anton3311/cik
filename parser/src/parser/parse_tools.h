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

IntegerLiteralInfo int_literal_info_from_token(Token token, Diagnostics* diagnostics);

bool parse_integer_literal_value(Diagnostics* diagnostics,
		Token literal_token,
		SourceString int_part_string,
		IntergerLiteralFormat format,
		uint64_t* out_result);

inline bool parse_integer_literal(Diagnostics* diagnostics, Token literal_token, uint64_t* out_result) {
	IntegerLiteralInfo literal_info = int_literal_info_from_token(literal_token, diagnostics);
	return parse_integer_literal_value(diagnostics,
			literal_token,
			literal_info.int_part_string,
			literal_info.format,
			out_result);
}

#endif
