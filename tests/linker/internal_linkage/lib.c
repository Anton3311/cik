#include "lib.h"

static uint64_t static_function() {
	return 50;
}

uint64_t call_other_static_function() {
	return static_function();
}
