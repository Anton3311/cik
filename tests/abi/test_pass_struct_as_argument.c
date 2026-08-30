#include <stdio.h>

__declspec(dllimport) void assert(unsigned long long);

typedef struct {
	size_t start;
	size_t end;
} Range;

size_t min(size_t a, size_t b) {
	if (a < b) { return a; }
	return b;
}

size_t max(size_t a, size_t b) {
	if (a > b) { return a; }
	return b;
}

size_t range_length(Range range) {
	return range.end - range.start;
}

Range range_merge(Range a, Range b) {
	return (Range) { min(a.start, b.start), max(a.end, b.end) };
}

int main(int argc, char *argv[]) {
	assert(range_length((Range) { 10, 20 }) == 10);

	Range merged = range_merge((Range) { 10, 20 }, (Range) { 3, 14 });
	assert(merged.start == 3);
	assert(merged.end == 20);
	return 0;
}
