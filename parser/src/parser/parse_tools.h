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

bool parse_integer_literal_value(Diagnostics* diagnostics,
		Token literal_token,
		String string,
		IntergerLiteralFormat format,
		uint64_t* out_result);

#endif
