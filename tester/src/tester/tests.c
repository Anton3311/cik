#include "tests.h"

void test_non_function_style_macro_expansion(TestContext* context) {
	assert(true);
}

void test_fail(TestContext* context) {
	assert(false);
}

void test_success(TestContext* context) {
	assert(true);
}

