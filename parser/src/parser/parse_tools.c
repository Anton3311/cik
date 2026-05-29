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
		SourceString literal_string,
		size_t invalid_char_position,
		IntergerLiteralFormat format) {

	String format_string = int_literal_format_to_string(format);
	String invalid_char_sub_str = sub_str(literal_string.string, invalid_char_position, 1);

	StringBuilder builder = { .arena = diagnostics->allocator };
	str_builder_append(&builder, STR_LIT("Unexpected char in "));
	str_builder_append(&builder, format_string);
	str_builder_append(&builder, STR_LIT(" integer literal"));

	diagnostics_report_error(diagnostics,
			source_string_to_range((SourceString) {
				.string = invalid_char_sub_str,
				.source_file = literal_string.source_file,
			}),
			builder.string,
			NULL);
}

IntegerLiteralInfo int_literal_info_from_token(Token token, Diagnostics* diagnostics) {
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

	assert(literal_string.length > 0);

	IntegerLiteralInfo info = (IntegerLiteralInfo) {
		.format = format,
		.int_part_string = (SourceString) {
			.source_file = token.source_range.source_file,
			.string = literal_string,
		},
	};

	// First parse these bit-count sufixes, which are microsoft specific
	//
	// 100ui64
	//     ^^^- this part
	String bit_count_sufixes[] = {
		STR_LIT("8"),
		STR_LIT("16"),
		STR_LIT("32"),
		STR_LIT("64"),
	};

	size_t bit_count_part_length = 0;
	for (size_t i = 0; i < array_size(bit_count_sufixes); i += 1) {
		size_t sufix_length_with_i = bit_count_sufixes[i].length + 1; // length including `i` or `I` char
		if (literal_string.length < sufix_length_with_i) {
			continue;
		}

		String literal_end = sub_str(literal_string,
				literal_string.length - bit_count_sufixes[i].length,
				bit_count_sufixes[i].length);

		if (!str_equal(literal_end, bit_count_sufixes[i])) {
			continue;
		}

		size_t i_char_pos = literal_string.length - bit_count_sufixes[i].length - 1;
		if (tolower(literal_string.v[i_char_pos]) == 'i') {
			info.has_sufix = true;

			uint8_t bit_counts[] = { 8, 16, 32, 64 };
			info.sufix_bit_count = bit_counts[i];
			bit_count_part_length = bit_count_sufixes[i].length + 1; // include the `i` char
			break;
		}
	}

	// Now parse the standard sufixes: u, ul, ll and ull
	//
	// 100ui64
	//    ^ - this part
	String std_sufixes[] = {
		STR_LIT("u"),
		STR_LIT("l"),
		STR_LIT("ul"),
		STR_LIT("ll"),
		STR_LIT("ull"),
	};

	IntegerLiteralSufixKind std_sufix_kinds[] = {
		INT_SUFIX_U,
		INT_SUFIX_L,
		INT_SUFIX_UL,
		INT_SUFIX_LL,
		INT_SUFIX_ULL,
	};

	size_t final_sufix_length = bit_count_part_length;
	for (size_t i = 0; i < array_size(std_sufixes); i += 1) {
		size_t full_sufix_length = std_sufixes[i].length + bit_count_part_length;

		if (literal_string.length <= full_sufix_length) {
			// No way that literal has this sufix
			continue;
		}

		size_t start = literal_string.length - full_sufix_length;
		bool sufix_matches = true;
		for (size_t j = 0; j < std_sufixes[i].length; j += 1) {
			if (tolower(literal_string.v[start + j]) != std_sufixes[i].v[j]) {
				sufix_matches = false;
				break;
			}
		}

		if (sufix_matches) {
			info.has_sufix = true;
			info.sufix_kind = std_sufix_kinds[i];

			final_sufix_length = full_sufix_length;
		}
	}
	
	if (info.has_sufix) {
		if (info.sufix_bit_count != 0
				&& (info.sufix_kind != INT_SUFIX_NONE && info.sufix_kind != INT_SUFIX_U)) {
			// We have an invalid sufix combination

			info.has_sufix = false;
			info.sufix_kind = INT_SUFIX_NONE;
			info.sufix_bit_count = 0;

			String sufix_string = sub_str(info.int_part_string.string,
					info.int_part_string.string.length - final_sufix_length,
					final_sufix_length);

			diagnostics_report_error(diagnostics,
					source_string_to_range((SourceString) {
						.string = sufix_string,
						.source_file = token.source_range.source_file,
					}),
					STR_LIT("Invalid sufix on integer"),
					NULL);
		}
	}

	info.int_part_string.string.length -= final_sufix_length;
	return info;
}

bool parse_integer_literal_value(Diagnostics* diagnostics,
		Token literal_token,
		SourceString int_part_string,
		IntergerLiteralFormat format,
		uint64_t* out_result) {
	if (int_part_string.string.length == 0) {
		diagnostics_report_error(diagnostics,
				literal_token.source_range,
				STR_LIT("Invalid integer literal"),
				NULL);
		return false;
	}

	String string = int_part_string.string;
	uint64_t result = 0;

	switch (format) {
	case INT_LIT_FMT_DECIMAL:
		for (size_t i = 0; i < string.length; i += 1) {
			if (string.v[i] >= '0' && string.v[i] <= '9') {
				uint64_t digit = (uint64_t)(string.v[i] - '0');
				result = result * 10 + digit;
			} else {
				_report_invalid_char_in_interger_literal(diagnostics, int_part_string, i, format);
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
				_report_invalid_char_in_interger_literal(diagnostics, int_part_string, i, format);
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
				_report_invalid_char_in_interger_literal(diagnostics, int_part_string, i, format);
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
