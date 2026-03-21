#include "parse_tools.h"

String int_literal_format_to_string(IntergerLiteralFormat format) {
	switch (format) {
	case INT_LIT_FMT_DECIMAL:
		return STR_LIT("decimal");
	case INT_LIT_FMT_BIN:
		return STR_LIT("binary");
	case INT_LIT_FMT_OCTAL:
		return STR_LIT("octal");
	case INT_LIT_FMT_HEX:
		return STR_LIT("hexidecimal");
	}

	unreachable();
	return (String){};
}

void _report_invalid_char_in_interger_literal(Diagnostics* diagnostics,
		String literal_string,
		size_t invalid_char_position,
		IntergerLiteralFormat format) {

	String format_string = int_literal_format_to_string(format);
	String invalid_char_sub_str = sub_str(literal_string, invalid_char_position, 1);

	StringBuilder builder = { .arena = diagnostics->allocator };
	str_builder_append(&builder, STR_LIT("Unexpected char in "));
	str_builder_append(&builder, format_string);
	str_builder_append(&builder, STR_LIT(" integer literal"));

	diagnostics_report_error(diagnostics,
			source_range_from_sub_string(diagnostics->source_code, invalid_char_sub_str),
			builder.string,
			NULL);
}

IntegerLiteralInfo int_literal_info_from_token(Token token) {
	IntergerLiteralFormat format = INT_LIT_FMT_DECIMAL;
	String literal_string = token.string;

	if (token.string.v[0] == '0') {
		bool prefix_parsed = false;
		if (token.string.length > 2) {
			if (token.string.v[1] == 'x') {
				format = INT_LIT_FMT_HEX;
				literal_string.v += 2;
				literal_string.length -= 2;
				prefix_parsed = true;
			} else if (token.string.v[1] == 'b') {
				format = INT_LIT_FMT_BIN;
				literal_string.v += 2;
				literal_string.length -= 2;
				prefix_parsed = true;
			}
		}

		if (!prefix_parsed) {

			// NOTE: Avoid discarding the leading zero, if the literal is just a zero
			if (token.string.length > 1) {
				format = INT_LIT_FMT_OCTAL;
				literal_string.v += 1;
				literal_string.length -= 1;
			}

			prefix_parsed = true;
		}

		assert(prefix_parsed);
	}

	return (IntegerLiteralInfo) {
		.format = format,
		.int_part_string = literal_string
	};
}

bool parse_integer_literal_value(Diagnostics* diagnostics,
		Token literal_token,
		String string,
		IntergerLiteralFormat format,
		uint64_t* out_result) {
	if (string.length == 0) {
		diagnostics_report_error(diagnostics,
				literal_token.source_range,
				STR_LIT("Invalid integer literal"),
				NULL);
		return false;
	}

	uint64_t result = 0;

	switch (format) {
	case INT_LIT_FMT_DECIMAL:
		for (size_t i = 0; i < string.length; i += 1) {
			if (string.v[i] >= '0' && string.v[i] <= '9') {
				uint64_t digit = (uint64_t)(string.v[i] - '0');
				result = result * 10 + digit;
			} else {
				_report_invalid_char_in_interger_literal(diagnostics, string, i, format);
				return false;
			}
		}

		*out_result = result;
		return true;
	case INT_LIT_FMT_BIN:
	case INT_LIT_FMT_OCTAL: {
		uint64_t shift = 0;
		char char_range_end = 0;
		
		if (format == INT_LIT_FMT_BIN) {
			shift = 1;
			char_range_end = '1';
		} else if (format == INT_LIT_FMT_OCTAL) {
			shift = 3;
			char_range_end = '7';
		} else {
			unreachable();
		}

		for (size_t i = 0; i < string.length; i += 1) {
			if (string.v[i] >= '0' && string.v[i] <= char_range_end) {
				uint64_t digit = (uint64_t)(string.v[i] - '0');
				result = result << shift | digit;
			} else {
				_report_invalid_char_in_interger_literal(diagnostics, string, i, format);
				return false;
			}
		}

		*out_result = result;
		return true;
	}
	case INT_LIT_FMT_HEX: {
		for (size_t i = 0; i < string.length; i += 1) {
			uint64_t digit;
			if (string.v[i] >= '0' && string.v[i] <= '9') {
				digit = (uint64_t)(string.v[i] - '0');
			} else if (string.v[i] >= 'a' && string.v[i] <= 'f') {
				digit = (uint64_t)(string.v[i] - 'a' + 10);
			} else if (string.v[i] >= 'A' && string.v[i] <= 'F') {
				digit = (uint64_t)(string.v[i] - 'A' + 10);
			} else {
				_report_invalid_char_in_interger_literal(diagnostics, string, i, format);
				return false;
			}

			result = result << 4 | digit;
		}

		*out_result = result;
		return true;
	}
	}

	unreachable();
	return false;
}
