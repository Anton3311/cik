#ifndef TESTS_X64_H
#define TESTS_X64_H

#include "tester/tester_core.h"

void test_return_uint64_zero(TestContext* context);
void test_add_uint64_consts(TestContext* context);
void test_return_first_arg(TestContext* context);
void test_return_sum_of_first_two_args(TestContext* context);
void test_deref_function_arg(TestContext* context);
void test_index_arary_with_pointer_arithmetics(TestContext* context);
void test_index_arary_with_pointer_arithmetics_2(TestContext* context);

// Compare Tests
void test_compare_equal_two_uint64(TestContext* context);
void test_compare_equal_less_for_uint64(TestContext* context);
void test_compare_equal_greater_for_uint64(TestContext* context);

// Other Tests
void test_mutate_argument(TestContext* context);

// Condition Tests
void test_call_inside_inner_scope(TestContext* context);
void test_conditional_call_1(TestContext* context);
void test_conditional_call_2(TestContext* context);
void test_conditional_call_between_two_calls_1(TestContext* context);
void test_conditional_call_between_two_calls_2(TestContext* context);

// Phi Node Tests
void test_return_one_phi_node_1(TestContext* context);
void test_return_one_phi_node_2(TestContext* context);
void test_return_sum_of_phi_node_values(TestContext* context);
void test_phi_in_nested_if_else(TestContext* context);
void test_phi_in_if_without_else(TestContext* context);
void test_phi_in_nested_if_without_else(TestContext* context);

#endif
