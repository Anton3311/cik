//           THIS FILE WAS GENERATED
// ANY MANUAL MODIFICATIONS WILL BE DESCARDED
//
// The generator is implemented in:
// File: gen/src/gen/gen_main.c
// Function: _generate_instr

#include "instr.h"
static String s_instr_kind_to_string[] = {
    [INSTR_NO_OP] = STR_LIT("no_op"),
    [INSTR_CONST_8] = STR_LIT("const_8"),
    [INSTR_CONST_16] = STR_LIT("const_16"),
    [INSTR_CONST_32] = STR_LIT("const_32"),
    [INSTR_CONST_64] = STR_LIT("const_64"),
    [INSTR_BIN_OP_8] = STR_LIT("bin_op_8"),
    [INSTR_BIN_OP_16] = STR_LIT("bin_op_16"),
    [INSTR_BIN_OP_32] = STR_LIT("bin_op_32"),
    [INSTR_BIN_OP_64] = STR_LIT("bin_op_64"),
    [INSTR_LOGICAL_SHIFT_LEFT_8] = STR_LIT("logical_shift_left_8"),
    [INSTR_LOGICAL_SHIFT_LEFT_16] = STR_LIT("logical_shift_left_16"),
    [INSTR_LOGICAL_SHIFT_LEFT_32] = STR_LIT("logical_shift_left_32"),
    [INSTR_LOGICAL_SHIFT_LEFT_64] = STR_LIT("logical_shift_left_64"),
    [INSTR_LOGICAL_SHIFT_RIGHT_8] = STR_LIT("logical_shift_right_8"),
    [INSTR_LOGICAL_SHIFT_RIGHT_16] = STR_LIT("logical_shift_right_16"),
    [INSTR_LOGICAL_SHIFT_RIGHT_32] = STR_LIT("logical_shift_right_32"),
    [INSTR_LOGICAL_SHIFT_RIGHT_64] = STR_LIT("logical_shift_right_64"),
    [INSTR_COMPARE_8] = STR_LIT("compare_8"),
    [INSTR_COMPARE_16] = STR_LIT("compare_16"),
    [INSTR_COMPARE_32] = STR_LIT("compare_32"),
    [INSTR_COMPARE_64] = STR_LIT("compare_64"),
    [INSTR_CAST_TO_8] = STR_LIT("cast_to_8"),
    [INSTR_CAST_TO_16] = STR_LIT("cast_to_16"),
    [INSTR_CAST_TO_32] = STR_LIT("cast_to_32"),
    [INSTR_CAST_TO_64] = STR_LIT("cast_to_64"),
    [INSTR_PTR_LOAD_8] = STR_LIT("ptr_load_8"),
    [INSTR_PTR_LOAD_16] = STR_LIT("ptr_load_16"),
    [INSTR_PTR_LOAD_32] = STR_LIT("ptr_load_32"),
    [INSTR_PTR_LOAD_64] = STR_LIT("ptr_load_64"),
    [INSTR_LOAD_ARG] = STR_LIT("load_arg"),
    [INSTR_BRANCH] = STR_LIT("branch"),
    [INSTR_JUMP] = STR_LIT("jump"),
    [INSTR_RETURN_VALUE] = STR_LIT("return_value"),
    [INSTR_IO_STATE] = STR_LIT("io_state"),
    [INSTR_REGION] = STR_LIT("region"),
    [INSTR_CALL_INTERNAL] = STR_LIT("call_internal"),
};
static String s_instr_bin_op_kind_to_string[] = {
    [INSTR_BIN_ADD] = STR_LIT("add"),
    [INSTR_BIN_SUB] = STR_LIT("sub"),
    [INSTR_BIN_MUL] = STR_LIT("mul"),
    [INSTR_BIN_DIV] = STR_LIT("div"),
};
static String s_instr_compare_kind_to_string[] = {
    [INSTR_CMP_EQUAL] = STR_LIT("equal"),
    [INSTR_CMP_NOT_EQUAL] = STR_LIT("not_equal"),
    [INSTR_CMP_LESS] = STR_LIT("less"),
    [INSTR_CMP_GREATER] = STR_LIT("greater"),
};
String instr_name(InstrKind instr_kind) {
	return s_instr_kind_to_string[instr_kind];
}
String instr_bin_op_name(InstrBinOp op_kind) {
	return s_instr_bin_op_kind_to_string[op_kind];
}
String instr_compare_kind_name(InstrCompareKind kind) {
	return s_instr_compare_kind_to_string[kind];
}
void instr_enumerate_dependencies(const InstrBuffer buffer,
                                  InstrIndex instr_index,
                                  InstrStack* out_dependencies) {
    const Instr* instr = &buffer.instr[instr_index.value];
    switch (instr->kind) {
    case INSTR_NO_OP:
        break;
    case INSTR_CONST_8:
        break;
    case INSTR_CONST_16:
        break;
    case INSTR_CONST_32:
        break;
    case INSTR_CONST_64:
        break;
    case INSTR_BIN_OP_8:
        instr_stack_push(out_dependencies, instr->bin_op.left);
        instr_stack_push(out_dependencies, instr->bin_op.right);
        break;
    case INSTR_BIN_OP_16:
        instr_stack_push(out_dependencies, instr->bin_op.left);
        instr_stack_push(out_dependencies, instr->bin_op.right);
        break;
    case INSTR_BIN_OP_32:
        instr_stack_push(out_dependencies, instr->bin_op.left);
        instr_stack_push(out_dependencies, instr->bin_op.right);
        break;
    case INSTR_BIN_OP_64:
        instr_stack_push(out_dependencies, instr->bin_op.left);
        instr_stack_push(out_dependencies, instr->bin_op.right);
        break;
    case INSTR_LOGICAL_SHIFT_LEFT_8:
        instr_stack_push(out_dependencies, instr->logical_shift.operand);
        break;
    case INSTR_LOGICAL_SHIFT_LEFT_16:
        instr_stack_push(out_dependencies, instr->logical_shift.operand);
        break;
    case INSTR_LOGICAL_SHIFT_LEFT_32:
        instr_stack_push(out_dependencies, instr->logical_shift.operand);
        break;
    case INSTR_LOGICAL_SHIFT_LEFT_64:
        instr_stack_push(out_dependencies, instr->logical_shift.operand);
        break;
    case INSTR_LOGICAL_SHIFT_RIGHT_8:
        instr_stack_push(out_dependencies, instr->logical_shift.operand);
        break;
    case INSTR_LOGICAL_SHIFT_RIGHT_16:
        instr_stack_push(out_dependencies, instr->logical_shift.operand);
        break;
    case INSTR_LOGICAL_SHIFT_RIGHT_32:
        instr_stack_push(out_dependencies, instr->logical_shift.operand);
        break;
    case INSTR_LOGICAL_SHIFT_RIGHT_64:
        instr_stack_push(out_dependencies, instr->logical_shift.operand);
        break;
    case INSTR_COMPARE_8:
        instr_stack_push(out_dependencies, instr->compare.left);
        instr_stack_push(out_dependencies, instr->compare.right);
        break;
    case INSTR_COMPARE_16:
        instr_stack_push(out_dependencies, instr->compare.left);
        instr_stack_push(out_dependencies, instr->compare.right);
        break;
    case INSTR_COMPARE_32:
        instr_stack_push(out_dependencies, instr->compare.left);
        instr_stack_push(out_dependencies, instr->compare.right);
        break;
    case INSTR_COMPARE_64:
        instr_stack_push(out_dependencies, instr->compare.left);
        instr_stack_push(out_dependencies, instr->compare.right);
        break;
    case INSTR_CAST_TO_8:
        instr_stack_push(out_dependencies, instr->cast.value);
        break;
    case INSTR_CAST_TO_16:
        instr_stack_push(out_dependencies, instr->cast.value);
        break;
    case INSTR_CAST_TO_32:
        instr_stack_push(out_dependencies, instr->cast.value);
        break;
    case INSTR_CAST_TO_64:
        instr_stack_push(out_dependencies, instr->cast.value);
        break;
    case INSTR_PTR_LOAD_8:
        instr_stack_push(out_dependencies, instr->ptr_load.ptr);
        break;
    case INSTR_PTR_LOAD_16:
        instr_stack_push(out_dependencies, instr->ptr_load.ptr);
        break;
    case INSTR_PTR_LOAD_32:
        instr_stack_push(out_dependencies, instr->ptr_load.ptr);
        break;
    case INSTR_PTR_LOAD_64:
        instr_stack_push(out_dependencies, instr->ptr_load.ptr);
        break;
    case INSTR_LOAD_ARG:
        break;
    case INSTR_BRANCH:
        instr_stack_push(out_dependencies, instr->branch.condition);
        instr_stack_push(out_dependencies, instr->branch.true_region);
        instr_stack_push(out_dependencies, instr->branch.false_region);
        break;
    case INSTR_JUMP:
        instr_stack_push(out_dependencies, instr->jump.target_region);
        break;
    case INSTR_RETURN_VALUE:
        instr_stack_push(out_dependencies, instr->return_value.value);
        break;
    case INSTR_IO_STATE:
        instr_stack_push(out_dependencies, instr->io_state.producer);
        break;
    case INSTR_REGION:
        instr_stack_push(out_dependencies, instr->region.last_instr);
        instr_stack_push(out_dependencies, instr->region.io_state);
        break;
    case INSTR_CALL_INTERNAL:
        instr_stack_push(out_dependencies, instr->call_internal.arg);
        instr_stack_push(out_dependencies, instr->call_internal.io_state);
        break;
    case INSTR_COUNT:
        unreachable();
    }
}
void instr_print(const Instr* instr) {
    String name = instr_name(instr->kind);

    size_t name_width = 23;

    printf("\033[32;1m%.*s\033[0m \033[%uC", STR_FMT(name), (uint32_t)(name_width - name.length));

    switch (instr->kind) {
    case INSTR_NO_OP:
        break;
    case INSTR_CONST_8:
        printf("u: %u i: %d ", (uint32_t)instr->const_8.u, (int32_t)instr->const_8.i);
        break;
    case INSTR_CONST_16:
        printf("u: %u i: %d ", (uint32_t)instr->const_16.u, (int32_t)instr->const_16.i);
        break;
    case INSTR_CONST_32:
        printf("u: %u i: %d f: %f ", (uint32_t)instr->const_32.u, (int32_t)instr->const_32.i, instr->const_32.f);
        break;
    case INSTR_CONST_64:
        printf("u: %llu i: %lld f: %f ", instr->const_64.u, instr->const_64.i, instr->const_64.f);
        break;
    case INSTR_BIN_OP_8:
        printf("kind: %.*s left: %u right: %u ", STR_FMT(instr_bin_op_name(instr->bin_op.kind)), (uint32_t)instr->bin_op.left.value, (uint32_t)instr->bin_op.right.value);
        break;
    case INSTR_BIN_OP_16:
        printf("kind: %.*s left: %u right: %u ", STR_FMT(instr_bin_op_name(instr->bin_op.kind)), (uint32_t)instr->bin_op.left.value, (uint32_t)instr->bin_op.right.value);
        break;
    case INSTR_BIN_OP_32:
        printf("kind: %.*s left: %u right: %u ", STR_FMT(instr_bin_op_name(instr->bin_op.kind)), (uint32_t)instr->bin_op.left.value, (uint32_t)instr->bin_op.right.value);
        break;
    case INSTR_BIN_OP_64:
        printf("kind: %.*s left: %u right: %u ", STR_FMT(instr_bin_op_name(instr->bin_op.kind)), (uint32_t)instr->bin_op.left.value, (uint32_t)instr->bin_op.right.value);
        break;
    case INSTR_LOGICAL_SHIFT_LEFT_8:
        printf("operand: %u shift_count: %u ", (uint32_t)instr->logical_shift.operand.value, (uint32_t)instr->logical_shift.shift_count);
        break;
    case INSTR_LOGICAL_SHIFT_LEFT_16:
        printf("operand: %u shift_count: %u ", (uint32_t)instr->logical_shift.operand.value, (uint32_t)instr->logical_shift.shift_count);
        break;
    case INSTR_LOGICAL_SHIFT_LEFT_32:
        printf("operand: %u shift_count: %u ", (uint32_t)instr->logical_shift.operand.value, (uint32_t)instr->logical_shift.shift_count);
        break;
    case INSTR_LOGICAL_SHIFT_LEFT_64:
        printf("operand: %u shift_count: %u ", (uint32_t)instr->logical_shift.operand.value, (uint32_t)instr->logical_shift.shift_count);
        break;
    case INSTR_LOGICAL_SHIFT_RIGHT_8:
        printf("operand: %u shift_count: %u ", (uint32_t)instr->logical_shift.operand.value, (uint32_t)instr->logical_shift.shift_count);
        break;
    case INSTR_LOGICAL_SHIFT_RIGHT_16:
        printf("operand: %u shift_count: %u ", (uint32_t)instr->logical_shift.operand.value, (uint32_t)instr->logical_shift.shift_count);
        break;
    case INSTR_LOGICAL_SHIFT_RIGHT_32:
        printf("operand: %u shift_count: %u ", (uint32_t)instr->logical_shift.operand.value, (uint32_t)instr->logical_shift.shift_count);
        break;
    case INSTR_LOGICAL_SHIFT_RIGHT_64:
        printf("operand: %u shift_count: %u ", (uint32_t)instr->logical_shift.operand.value, (uint32_t)instr->logical_shift.shift_count);
        break;
    case INSTR_COMPARE_8:
        printf("kind: %.*s left: %u right: %u ", STR_FMT(instr_compare_kind_name(instr->compare.kind)), (uint32_t)instr->compare.left.value, (uint32_t)instr->compare.right.value);
        break;
    case INSTR_COMPARE_16:
        printf("kind: %.*s left: %u right: %u ", STR_FMT(instr_compare_kind_name(instr->compare.kind)), (uint32_t)instr->compare.left.value, (uint32_t)instr->compare.right.value);
        break;
    case INSTR_COMPARE_32:
        printf("kind: %.*s left: %u right: %u ", STR_FMT(instr_compare_kind_name(instr->compare.kind)), (uint32_t)instr->compare.left.value, (uint32_t)instr->compare.right.value);
        break;
    case INSTR_COMPARE_64:
        printf("kind: %.*s left: %u right: %u ", STR_FMT(instr_compare_kind_name(instr->compare.kind)), (uint32_t)instr->compare.left.value, (uint32_t)instr->compare.right.value);
        break;
    case INSTR_CAST_TO_8:
        printf("value: %u ", (uint32_t)instr->cast.value.value);
        break;
    case INSTR_CAST_TO_16:
        printf("value: %u ", (uint32_t)instr->cast.value.value);
        break;
    case INSTR_CAST_TO_32:
        printf("value: %u ", (uint32_t)instr->cast.value.value);
        break;
    case INSTR_CAST_TO_64:
        printf("value: %u ", (uint32_t)instr->cast.value.value);
        break;
    case INSTR_PTR_LOAD_8:
        printf("ptr: %u ", (uint32_t)instr->ptr_load.ptr.value);
        break;
    case INSTR_PTR_LOAD_16:
        printf("ptr: %u ", (uint32_t)instr->ptr_load.ptr.value);
        break;
    case INSTR_PTR_LOAD_32:
        printf("ptr: %u ", (uint32_t)instr->ptr_load.ptr.value);
        break;
    case INSTR_PTR_LOAD_64:
        printf("ptr: %u ", (uint32_t)instr->ptr_load.ptr.value);
        break;
    case INSTR_LOAD_ARG:
        printf("index: %u ", (uint32_t)instr->load_arg.index);
        break;
    case INSTR_BRANCH:
        printf("condition: %u true_region: %u false_region: %u ", (uint32_t)instr->branch.condition.value, (uint32_t)instr->branch.true_region.value, (uint32_t)instr->branch.false_region.value);
        break;
    case INSTR_JUMP:
        printf("target_region: %u ", (uint32_t)instr->jump.target_region.value);
        break;
    case INSTR_RETURN_VALUE:
        printf("value: %u ", (uint32_t)instr->return_value.value.value);
        break;
    case INSTR_IO_STATE:
        printf("producer: %u ", (uint32_t)instr->io_state.producer.value);
        break;
    case INSTR_REGION:
        printf("last_instr: %u io_state: %u ", (uint32_t)instr->region.last_instr.value, (uint32_t)instr->region.io_state.value);
        break;
    case INSTR_CALL_INTERNAL:
        printf("arg: %u io_state: %u function_index: %u ", (uint32_t)instr->call_internal.arg.value, (uint32_t)instr->call_internal.io_state.value, (uint32_t)instr->call_internal.function_index);
        break;
    case INSTR_COUNT:
        unreachable();
    }
    printf("\n");
}
