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

// Parse int literal prefixes: 0x, 0 (for octal), and 0b
// If the token is just a single `0` char, keeps it as decimal.
//
// Returns a literal string with the prefix removed
// The parsed prefix is stored in the provided `IntLiteral`
static String _parse_int_literal_prefix(String string, IntLiteral* out_result) {
	IntergerLiteralFormat format = INT_LIT_FMT_DECIMAL;

	if (string.v[0] == '0') {
		bool prefix_parsed = false;
		if (string.length > 2) {
			if (string.v[1] == 'x') {
				format = INT_LIT_FMT_HEX;
				string.v += 2;
				string.length -= 2;
				prefix_parsed = true;
			} else if (string.v[1] == 'b') {
				format = INT_LIT_FMT_BIN;
				string.v += 2;
				string.length -= 2;
				prefix_parsed = true;
			}
		}

		if (!prefix_parsed) {

			// NOTE: Avoid discarding the leading zero, if the literal is just a zero
			if (string.length > 1) {
				format = INT_LIT_FMT_OCTAL;
				string.v += 1;
				string.length -= 1;
			}

			prefix_parsed = true;
		}

		assert(prefix_parsed);
	}

	assert(string.length > 0);

	out_result->format = format;
	return string;
}

typedef struct {
	uint8_t sufix_length;
bool is_valid;
} SufixParseResult;

// Returns a literal string with the sufix removed, if parsed any
static SufixParseResult _parse_int_literal_sufix(String literal_string,
		const SourceFile* source_file,
		Diagnostics* diagnostics,
		IntLiteral* out_result) {

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
			out_result->has_sufix = true;

			uint8_t bit_counts[] = { 8, 16, 32, 64 };
			out_result->sufix_bit_count = bit_counts[i];
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
			out_result->has_sufix = true;
			out_result->sufix_kind = std_sufix_kinds[i];

			final_sufix_length = full_sufix_length;
		}
	}
	
	if (out_result->has_sufix) {
		if (out_result->sufix_bit_count != 0
				&& (out_result->sufix_kind != INT_SUFIX_NONE && out_result->sufix_kind != INT_SUFIX_U)) {
			// We have an invalid sufix combination

			out_result->has_sufix = false;
			out_result->sufix_kind = INT_SUFIX_NONE;
			out_result->sufix_bit_count = 0;

			String sufix_string = sub_str(literal_string,
					literal_string.length - final_sufix_length,
					final_sufix_length);

			diagnostics_report_error(diagnostics,
					source_string_to_range((SourceString) {
						.string = sufix_string,
						.source_file = source_file,
					}),
					STR_LIT("Invalid sufix on integer"),
					NULL);

			return (SufixParseResult) {
				.sufix_length = (uint8_t)final_sufix_length,
				.is_valid = false,
			};
		}
	}

	return (SufixParseResult) {
		.sufix_length = (uint8_t)final_sufix_length,
		.is_valid = true,
	};
}

static void _report_invalid_char_in_interger_literal(Diagnostics* diagnostics,
		String literal_string,
		const SourceFile* source_file,
		size_t invalid_char_position,
		IntergerLiteralFormat format) {

	String format_string = int_literal_format_to_string(format);
	String invalid_char_sub_str = sub_str(literal_string, invalid_char_position, 1);

	StringBuilder builder = { .arena = diagnostics->allocator };
	str_builder_append(&builder, STR_LIT("Unexpected char in "));
	str_builder_append(&builder, format_string);
	str_builder_append(&builder, STR_LIT(" integer literal"));

	diagnostics_report_error(diagnostics,
			source_string_to_range((SourceString) {
				.string = invalid_char_sub_str,
				.source_file = source_file,
			}),
			builder.string,
			NULL);
}

static bool _parse_int_literal_value(Diagnostics* diagnostics,
		Token literal_token,
		String string,
		const SourceFile* source_file,
		IntergerLiteralFormat format,
		uint64_t* out_result) {
	if (string.length == 0) {
		diagnostics_report_error(diagnostics,
				source_string_to_range((SourceString) {
					.string = string,
					.source_file = source_file,
				}),
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
				_report_invalid_char_in_interger_literal(diagnostics, string, source_file, i, format);
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
				_report_invalid_char_in_interger_literal(diagnostics, string, source_file, i, format);
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
				_report_invalid_char_in_interger_literal(diagnostics, string, source_file, i, format);
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

bool parse_int_literal(Token token, Diagnostics* diagnostics, IntLiteral* out_result) {
	SufixParseResult sufix_result = _parse_int_literal_sufix(token.string,
			token.source_range.source_file,
			diagnostics,
			out_result);

	if (!sufix_result.is_valid) {
		return false;
	}

	String literal_without_sufix = token.string;
	assert(sufix_result.sufix_length <= literal_without_sufix.length);
	literal_without_sufix.length -= sufix_result.sufix_length;

	String digits = _parse_int_literal_prefix(literal_without_sufix, out_result);
	assert(digits.length > 0);

	return _parse_int_literal_value(diagnostics,
			token,
			digits,
			token.source_range.source_file,
			out_result->format,
			&out_result->value);
}
