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

typedef struct {
	IntergerLiteralFormat format;
	SourceString int_part_string;
} IntegerLiteralInfo;

IntegerLiteralInfo int_literal_info_from_token(Token token);

bool parse_integer_literal_value(Diagnostics* diagnostics,
		Token literal_token,
		SourceString int_part_string,
		IntergerLiteralFormat format,
		uint64_t* out_result);

inline bool parse_integer_literal(Diagnostics* diagnostics, Token literal_token, uint64_t* out_result) {
	IntegerLiteralInfo literal_info = int_literal_info_from_token(literal_token);
	return parse_integer_literal_value(diagnostics,
			literal_token,
			literal_info.int_part_string,
			literal_info.format,
			out_result);
}

#endif
