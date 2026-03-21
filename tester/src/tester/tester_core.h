#ifndef TESTER_CORE
#define TESTER_CORE

#include "core/core.h"

typedef enum {
	TEST_CMD_GET_TEST_SUITE_COUNT,
	TEST_CMD_GET_TEST_SUITE_NAMES,

	TEST_CMD_GET_TEST_COUNT,
	TEST_CMD_GET_TEST_NAMES,

	TEST_CMD_RUN_TEST,

	TEST_CMD_RUN_PREPROCESSOR_TEST,

	TEST_CMD_COUNT,
} TestCommandKind;

typedef struct {
	Arena* arena;
	Arena* temp_arena;
} TestContext;

typedef void(*TestFunction)(TestContext* context);

typedef struct {
	String name;
	TestFunction function;
} TestCase;

typedef struct {
	String name;
	TestCase* cases;
	size_t case_count;
} TestSuite;

#define test(test_name) (TestCase) { .name = STR_LIT(#test_name), .function = test_name }
#define test_suite(suite_name) (TestSuite) { \
	.name = STR_LIT(#suite_name), \
	.cases = suite_name, \
	.case_count = array_size(suite_name) \
}


#endif
