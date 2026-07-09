//           THIS FILE WAS GENERATED
// ANY MANUAL MODIFICATIONS WILL BE DESCARDED
//
// The generator is implemented in:
// File: gen/src/gen/gen_main.c
// Function: _generate_instr

#include "instr.h"
String instr_name(InstrKind variant) {
    switch (variant) {
    case INSTR_NO_OP: return STR_LIT("no_op");
    case INSTR_CONST_8: return STR_LIT("const_8");
    case INSTR_CONST_16: return STR_LIT("const_16");
    case INSTR_CONST_32: return STR_LIT("const_32");
    case INSTR_CONST_64: return STR_LIT("const_64");
    case INSTR_CONST_STRING: return STR_LIT("const_string");
    case INSTR_BIN_OP_8: return STR_LIT("bin_op_8");
    case INSTR_BIN_OP_16: return STR_LIT("bin_op_16");
    case INSTR_BIN_OP_32: return STR_LIT("bin_op_32");
    case INSTR_BIN_OP_64: return STR_LIT("bin_op_64");
    case INSTR_NEGATE_8: return STR_LIT("negate_8");
    case INSTR_NEGATE_16: return STR_LIT("negate_16");
    case INSTR_NEGATE_32: return STR_LIT("negate_32");
    case INSTR_NEGATE_64: return STR_LIT("negate_64");
    case INSTR_LOGICAL_SHIFT_LEFT_8: return STR_LIT("logical_shift_left_8");
    case INSTR_LOGICAL_SHIFT_LEFT_16: return STR_LIT("logical_shift_left_16");
    case INSTR_LOGICAL_SHIFT_LEFT_32: return STR_LIT("logical_shift_left_32");
    case INSTR_LOGICAL_SHIFT_LEFT_64: return STR_LIT("logical_shift_left_64");
    case INSTR_LOGICAL_SHIFT_RIGHT_8: return STR_LIT("logical_shift_right_8");
    case INSTR_LOGICAL_SHIFT_RIGHT_16: return STR_LIT("logical_shift_right_16");
    case INSTR_LOGICAL_SHIFT_RIGHT_32: return STR_LIT("logical_shift_right_32");
    case INSTR_LOGICAL_SHIFT_RIGHT_64: return STR_LIT("logical_shift_right_64");
    case INSTR_COMPARE_8: return STR_LIT("compare_8");
    case INSTR_COMPARE_16: return STR_LIT("compare_16");
    case INSTR_COMPARE_32: return STR_LIT("compare_32");
    case INSTR_COMPARE_64: return STR_LIT("compare_64");
    case INSTR_BOOL_TO_INT: return STR_LIT("bool_to_int");
    case INSTR_CAST_TO_8: return STR_LIT("cast_to_8");
    case INSTR_CAST_TO_16: return STR_LIT("cast_to_16");
    case INSTR_CAST_TO_32: return STR_LIT("cast_to_32");
    case INSTR_CAST_TO_64: return STR_LIT("cast_to_64");
    case INSTR_PTR_LOAD_8: return STR_LIT("ptr_load_8");
    case INSTR_PTR_LOAD_16: return STR_LIT("ptr_load_16");
    case INSTR_PTR_LOAD_32: return STR_LIT("ptr_load_32");
    case INSTR_PTR_LOAD_64: return STR_LIT("ptr_load_64");
    case INSTR_PTR_STORE_8: return STR_LIT("ptr_store_8");
    case INSTR_PTR_STORE_16: return STR_LIT("ptr_store_16");
    case INSTR_PTR_STORE_32: return STR_LIT("ptr_store_32");
    case INSTR_PTR_STORE_64: return STR_LIT("ptr_store_64");
    case INSTR_LOAD_ARG: return STR_LIT("load_arg");
    case INSTR_BRANCH: return STR_LIT("branch");
    case INSTR_JUMP: return STR_LIT("jump");
    case INSTR_RET: return STR_LIT("ret");
    case INSTR_RETURN_VALUE: return STR_LIT("return_value");
    case INSTR_IO_STATE: return STR_LIT("io_state");
    case INSTR_REGION: return STR_LIT("region");
    case INSTR_PHI: return STR_LIT("phi");
    case INSTR_SELECT: return STR_LIT("select");
    case INSTR_CALL_INTERNAL: return STR_LIT("call_internal");
    }
    unreachable();
    return (String) {};
}
String instr_bin_op_name(InstrBinOp variant) {
    switch (variant) {
    case INSTR_BIN_ADD: return STR_LIT("add");
    case INSTR_BIN_SUB: return STR_LIT("sub");
    case INSTR_BIN_MUL: return STR_LIT("mul");
    case INSTR_BIN_DIV: return STR_LIT("div");
    }
    unreachable();
    return (String) {};
}
String instr_compare_kind_name(InstrCompareKind variant) {
    switch (variant) {
    case INSTR_CMP_EQUAL: return STR_LIT("equal");
    case INSTR_CMP_NOT_EQUAL: return STR_LIT("not_equal");
    case INSTR_CMP_LESS: return STR_LIT("less");
    case INSTR_CMP_LESS_OR_EQUAL: return STR_LIT("less_or_equal");
    case INSTR_CMP_GREATER: return STR_LIT("greater");
    case INSTR_CMP_GREATER_OR_EQUAL: return STR_LIT("greater_or_equal");
    }
    unreachable();
    return (String) {};
}
void instr_enumerate_uses(const InstrBuffer* buffer,
                                InstrIndex instr_index,
                                InstrQueue* out_dependencies) {
    const Instr* instr = &buffer->instr[instr_index.value];
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
    case INSTR_CONST_STRING:
        break;
    case INSTR_BIN_OP_8:
        instr_queue_push_back(out_dependencies, instr->bin_op.left);
        instr_queue_push_back(out_dependencies, instr->bin_op.right);
        break;
    case INSTR_BIN_OP_16:
        instr_queue_push_back(out_dependencies, instr->bin_op.left);
        instr_queue_push_back(out_dependencies, instr->bin_op.right);
        break;
    case INSTR_BIN_OP_32:
        instr_queue_push_back(out_dependencies, instr->bin_op.left);
        instr_queue_push_back(out_dependencies, instr->bin_op.right);
        break;
    case INSTR_BIN_OP_64:
        instr_queue_push_back(out_dependencies, instr->bin_op.left);
        instr_queue_push_back(out_dependencies, instr->bin_op.right);
        break;
    case INSTR_NEGATE_8:
        instr_queue_push_back(out_dependencies, instr->negate.operand);
        break;
    case INSTR_NEGATE_16:
        instr_queue_push_back(out_dependencies, instr->negate.operand);
        break;
    case INSTR_NEGATE_32:
        instr_queue_push_back(out_dependencies, instr->negate.operand);
        break;
    case INSTR_NEGATE_64:
        instr_queue_push_back(out_dependencies, instr->negate.operand);
        break;
    case INSTR_LOGICAL_SHIFT_LEFT_8:
        instr_queue_push_back(out_dependencies, instr->logical_shift.operand);
        break;
    case INSTR_LOGICAL_SHIFT_LEFT_16:
        instr_queue_push_back(out_dependencies, instr->logical_shift.operand);
        break;
    case INSTR_LOGICAL_SHIFT_LEFT_32:
        instr_queue_push_back(out_dependencies, instr->logical_shift.operand);
        break;
    case INSTR_LOGICAL_SHIFT_LEFT_64:
        instr_queue_push_back(out_dependencies, instr->logical_shift.operand);
        break;
    case INSTR_LOGICAL_SHIFT_RIGHT_8:
        instr_queue_push_back(out_dependencies, instr->logical_shift.operand);
        break;
    case INSTR_LOGICAL_SHIFT_RIGHT_16:
        instr_queue_push_back(out_dependencies, instr->logical_shift.operand);
        break;
    case INSTR_LOGICAL_SHIFT_RIGHT_32:
        instr_queue_push_back(out_dependencies, instr->logical_shift.operand);
        break;
    case INSTR_LOGICAL_SHIFT_RIGHT_64:
        instr_queue_push_back(out_dependencies, instr->logical_shift.operand);
        break;
    case INSTR_COMPARE_8:
        instr_queue_push_back(out_dependencies, instr->compare.left);
        instr_queue_push_back(out_dependencies, instr->compare.right);
        break;
    case INSTR_COMPARE_16:
        instr_queue_push_back(out_dependencies, instr->compare.left);
        instr_queue_push_back(out_dependencies, instr->compare.right);
        break;
    case INSTR_COMPARE_32:
        instr_queue_push_back(out_dependencies, instr->compare.left);
        instr_queue_push_back(out_dependencies, instr->compare.right);
        break;
    case INSTR_COMPARE_64:
        instr_queue_push_back(out_dependencies, instr->compare.left);
        instr_queue_push_back(out_dependencies, instr->compare.right);
        break;
    case INSTR_BOOL_TO_INT:
        instr_queue_push_back(out_dependencies, instr->bool_to_int.operand);
        break;
    case INSTR_CAST_TO_8:
        instr_queue_push_back(out_dependencies, instr->cast.value);
        break;
    case INSTR_CAST_TO_16:
        instr_queue_push_back(out_dependencies, instr->cast.value);
        break;
    case INSTR_CAST_TO_32:
        instr_queue_push_back(out_dependencies, instr->cast.value);
        break;
    case INSTR_CAST_TO_64:
        instr_queue_push_back(out_dependencies, instr->cast.value);
        break;
    case INSTR_PTR_LOAD_8:
        instr_queue_push_back(out_dependencies, instr->ptr_load.ptr);
        instr_queue_push_back(out_dependencies, instr->ptr_load.io_state);
        break;
    case INSTR_PTR_LOAD_16:
        instr_queue_push_back(out_dependencies, instr->ptr_load.ptr);
        instr_queue_push_back(out_dependencies, instr->ptr_load.io_state);
        break;
    case INSTR_PTR_LOAD_32:
        instr_queue_push_back(out_dependencies, instr->ptr_load.ptr);
        instr_queue_push_back(out_dependencies, instr->ptr_load.io_state);
        break;
    case INSTR_PTR_LOAD_64:
        instr_queue_push_back(out_dependencies, instr->ptr_load.ptr);
        instr_queue_push_back(out_dependencies, instr->ptr_load.io_state);
        break;
    case INSTR_PTR_STORE_8:
        instr_queue_push_back(out_dependencies, instr->ptr_store.ptr);
        instr_queue_push_back(out_dependencies, instr->ptr_store.value);
        instr_queue_push_back(out_dependencies, instr->ptr_store.io_state);
        break;
    case INSTR_PTR_STORE_16:
        instr_queue_push_back(out_dependencies, instr->ptr_store.ptr);
        instr_queue_push_back(out_dependencies, instr->ptr_store.value);
        instr_queue_push_back(out_dependencies, instr->ptr_store.io_state);
        break;
    case INSTR_PTR_STORE_32:
        instr_queue_push_back(out_dependencies, instr->ptr_store.ptr);
        instr_queue_push_back(out_dependencies, instr->ptr_store.value);
        instr_queue_push_back(out_dependencies, instr->ptr_store.io_state);
        break;
    case INSTR_PTR_STORE_64:
        instr_queue_push_back(out_dependencies, instr->ptr_store.ptr);
        instr_queue_push_back(out_dependencies, instr->ptr_store.value);
        instr_queue_push_back(out_dependencies, instr->ptr_store.io_state);
        break;
    case INSTR_LOAD_ARG:
        break;
    case INSTR_BRANCH:
        instr_queue_push_back(out_dependencies, instr->branch.condition);
        instr_queue_push_back(out_dependencies, instr->branch.true_region);
        instr_queue_push_back(out_dependencies, instr->branch.false_region);
        instr_queue_push_back(out_dependencies, instr->branch.io_state);
        break;
    case INSTR_JUMP:
        instr_queue_push_back(out_dependencies, instr->jump.target_region);
        instr_queue_push_back(out_dependencies, instr->jump.io_state);
        break;
    case INSTR_RET:
        instr_queue_push_back(out_dependencies, instr->ret.io_state);
        break;
    case INSTR_RETURN_VALUE:
        instr_queue_push_back(out_dependencies, instr->return_value.value);
        instr_queue_push_back(out_dependencies, instr->return_value.io_state);
        break;
    case INSTR_IO_STATE:
        instr_queue_push_back(out_dependencies, instr->io_state.producer);
        break;
    case INSTR_REGION:
        instr_queue_push_back(out_dependencies, instr->region.last_instr);
        break;
    case INSTR_PHI:
        instr_push_input_dependencies(buffer, instr->phi.variants, out_dependencies);
        break;
    case INSTR_SELECT:
        instr_queue_push_back(out_dependencies, instr->select.value);
        instr_queue_push_back(out_dependencies, instr->select.region);
        break;
    case INSTR_CALL_INTERNAL:
        instr_push_input_dependencies(buffer, instr->call_internal.args, out_dependencies);
        instr_queue_push_back(out_dependencies, instr->call_internal.io_state);
        break;
    case INSTR_COUNT:
        unreachable();
    }
}
void instr_print(const Instr* instr, const InstrIndex* input_instr_buffer, Arena* temp_allocator) {
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
    case INSTR_CONST_STRING:
        printf("string_id: %u ", (uint32_t)instr->const_string.string_id);
        break;
    case INSTR_BIN_OP_8:
        printf("kind: %.*s left: \033[33;1m%%%u\033[0m right: \033[33;1m%%%u\033[0m ", STR_FMT(instr_bin_op_name(instr->bin_op.kind)), (uint32_t)instr->bin_op.left.value, (uint32_t)instr->bin_op.right.value);
        break;
    case INSTR_BIN_OP_16:
        printf("kind: %.*s left: \033[33;1m%%%u\033[0m right: \033[33;1m%%%u\033[0m ", STR_FMT(instr_bin_op_name(instr->bin_op.kind)), (uint32_t)instr->bin_op.left.value, (uint32_t)instr->bin_op.right.value);
        break;
    case INSTR_BIN_OP_32:
        printf("kind: %.*s left: \033[33;1m%%%u\033[0m right: \033[33;1m%%%u\033[0m ", STR_FMT(instr_bin_op_name(instr->bin_op.kind)), (uint32_t)instr->bin_op.left.value, (uint32_t)instr->bin_op.right.value);
        break;
    case INSTR_BIN_OP_64:
        printf("kind: %.*s left: \033[33;1m%%%u\033[0m right: \033[33;1m%%%u\033[0m ", STR_FMT(instr_bin_op_name(instr->bin_op.kind)), (uint32_t)instr->bin_op.left.value, (uint32_t)instr->bin_op.right.value);
        break;
    case INSTR_NEGATE_8:
        printf("operand: \033[33;1m%%%u\033[0m ", (uint32_t)instr->negate.operand.value);
        break;
    case INSTR_NEGATE_16:
        printf("operand: \033[33;1m%%%u\033[0m ", (uint32_t)instr->negate.operand.value);
        break;
    case INSTR_NEGATE_32:
        printf("operand: \033[33;1m%%%u\033[0m ", (uint32_t)instr->negate.operand.value);
        break;
    case INSTR_NEGATE_64:
        printf("operand: \033[33;1m%%%u\033[0m ", (uint32_t)instr->negate.operand.value);
        break;
    case INSTR_LOGICAL_SHIFT_LEFT_8:
        printf("operand: \033[33;1m%%%u\033[0m shift_count: %u ", (uint32_t)instr->logical_shift.operand.value, (uint32_t)instr->logical_shift.shift_count);
        break;
    case INSTR_LOGICAL_SHIFT_LEFT_16:
        printf("operand: \033[33;1m%%%u\033[0m shift_count: %u ", (uint32_t)instr->logical_shift.operand.value, (uint32_t)instr->logical_shift.shift_count);
        break;
    case INSTR_LOGICAL_SHIFT_LEFT_32:
        printf("operand: \033[33;1m%%%u\033[0m shift_count: %u ", (uint32_t)instr->logical_shift.operand.value, (uint32_t)instr->logical_shift.shift_count);
        break;
    case INSTR_LOGICAL_SHIFT_LEFT_64:
        printf("operand: \033[33;1m%%%u\033[0m shift_count: %u ", (uint32_t)instr->logical_shift.operand.value, (uint32_t)instr->logical_shift.shift_count);
        break;
    case INSTR_LOGICAL_SHIFT_RIGHT_8:
        printf("operand: \033[33;1m%%%u\033[0m shift_count: %u ", (uint32_t)instr->logical_shift.operand.value, (uint32_t)instr->logical_shift.shift_count);
        break;
    case INSTR_LOGICAL_SHIFT_RIGHT_16:
        printf("operand: \033[33;1m%%%u\033[0m shift_count: %u ", (uint32_t)instr->logical_shift.operand.value, (uint32_t)instr->logical_shift.shift_count);
        break;
    case INSTR_LOGICAL_SHIFT_RIGHT_32:
        printf("operand: \033[33;1m%%%u\033[0m shift_count: %u ", (uint32_t)instr->logical_shift.operand.value, (uint32_t)instr->logical_shift.shift_count);
        break;
    case INSTR_LOGICAL_SHIFT_RIGHT_64:
        printf("operand: \033[33;1m%%%u\033[0m shift_count: %u ", (uint32_t)instr->logical_shift.operand.value, (uint32_t)instr->logical_shift.shift_count);
        break;
    case INSTR_COMPARE_8:
        printf("kind: %.*s left: \033[33;1m%%%u\033[0m right: \033[33;1m%%%u\033[0m ", STR_FMT(instr_compare_kind_name(instr->compare.kind)), (uint32_t)instr->compare.left.value, (uint32_t)instr->compare.right.value);
        break;
    case INSTR_COMPARE_16:
        printf("kind: %.*s left: \033[33;1m%%%u\033[0m right: \033[33;1m%%%u\033[0m ", STR_FMT(instr_compare_kind_name(instr->compare.kind)), (uint32_t)instr->compare.left.value, (uint32_t)instr->compare.right.value);
        break;
    case INSTR_COMPARE_32:
        printf("kind: %.*s left: \033[33;1m%%%u\033[0m right: \033[33;1m%%%u\033[0m ", STR_FMT(instr_compare_kind_name(instr->compare.kind)), (uint32_t)instr->compare.left.value, (uint32_t)instr->compare.right.value);
        break;
    case INSTR_COMPARE_64:
        printf("kind: %.*s left: \033[33;1m%%%u\033[0m right: \033[33;1m%%%u\033[0m ", STR_FMT(instr_compare_kind_name(instr->compare.kind)), (uint32_t)instr->compare.left.value, (uint32_t)instr->compare.right.value);
        break;
    case INSTR_BOOL_TO_INT:
        printf("operand: \033[33;1m%%%u\033[0m ", (uint32_t)instr->bool_to_int.operand.value);
        break;
    case INSTR_CAST_TO_8:
        printf("value: \033[33;1m%%%u\033[0m ", (uint32_t)instr->cast.value.value);
        break;
    case INSTR_CAST_TO_16:
        printf("value: \033[33;1m%%%u\033[0m ", (uint32_t)instr->cast.value.value);
        break;
    case INSTR_CAST_TO_32:
        printf("value: \033[33;1m%%%u\033[0m ", (uint32_t)instr->cast.value.value);
        break;
    case INSTR_CAST_TO_64:
        printf("value: \033[33;1m%%%u\033[0m ", (uint32_t)instr->cast.value.value);
        break;
    case INSTR_PTR_LOAD_8:
        printf("ptr: \033[33;1m%%%u\033[0m io_state: \033[33;1m%%%u\033[0m ", (uint32_t)instr->ptr_load.ptr.value, (uint32_t)instr->ptr_load.io_state.value);
        break;
    case INSTR_PTR_LOAD_16:
        printf("ptr: \033[33;1m%%%u\033[0m io_state: \033[33;1m%%%u\033[0m ", (uint32_t)instr->ptr_load.ptr.value, (uint32_t)instr->ptr_load.io_state.value);
        break;
    case INSTR_PTR_LOAD_32:
        printf("ptr: \033[33;1m%%%u\033[0m io_state: \033[33;1m%%%u\033[0m ", (uint32_t)instr->ptr_load.ptr.value, (uint32_t)instr->ptr_load.io_state.value);
        break;
    case INSTR_PTR_LOAD_64:
        printf("ptr: \033[33;1m%%%u\033[0m io_state: \033[33;1m%%%u\033[0m ", (uint32_t)instr->ptr_load.ptr.value, (uint32_t)instr->ptr_load.io_state.value);
        break;
    case INSTR_PTR_STORE_8:
        printf("ptr: \033[33;1m%%%u\033[0m value: \033[33;1m%%%u\033[0m io_state: \033[33;1m%%%u\033[0m ", (uint32_t)instr->ptr_store.ptr.value, (uint32_t)instr->ptr_store.value.value, (uint32_t)instr->ptr_store.io_state.value);
        break;
    case INSTR_PTR_STORE_16:
        printf("ptr: \033[33;1m%%%u\033[0m value: \033[33;1m%%%u\033[0m io_state: \033[33;1m%%%u\033[0m ", (uint32_t)instr->ptr_store.ptr.value, (uint32_t)instr->ptr_store.value.value, (uint32_t)instr->ptr_store.io_state.value);
        break;
    case INSTR_PTR_STORE_32:
        printf("ptr: \033[33;1m%%%u\033[0m value: \033[33;1m%%%u\033[0m io_state: \033[33;1m%%%u\033[0m ", (uint32_t)instr->ptr_store.ptr.value, (uint32_t)instr->ptr_store.value.value, (uint32_t)instr->ptr_store.io_state.value);
        break;
    case INSTR_PTR_STORE_64:
        printf("ptr: \033[33;1m%%%u\033[0m value: \033[33;1m%%%u\033[0m io_state: \033[33;1m%%%u\033[0m ", (uint32_t)instr->ptr_store.ptr.value, (uint32_t)instr->ptr_store.value.value, (uint32_t)instr->ptr_store.io_state.value);
        break;
    case INSTR_LOAD_ARG:
        printf("index: %u ", (uint32_t)instr->load_arg.index);
        break;
    case INSTR_BRANCH:
        printf("condition: \033[33;1m%%%u\033[0m true_region: \033[33;1m%%%u\033[0m false_region: \033[33;1m%%%u\033[0m io_state: \033[33;1m%%%u\033[0m ", (uint32_t)instr->branch.condition.value, (uint32_t)instr->branch.true_region.value, (uint32_t)instr->branch.false_region.value, (uint32_t)instr->branch.io_state.value);
        break;
    case INSTR_JUMP:
        printf("target_region: \033[33;1m%%%u\033[0m io_state: \033[33;1m%%%u\033[0m ", (uint32_t)instr->jump.target_region.value, (uint32_t)instr->jump.io_state.value);
        break;
    case INSTR_RET:
        printf("io_state: \033[33;1m%%%u\033[0m ", (uint32_t)instr->ret.io_state.value);
        break;
    case INSTR_RETURN_VALUE:
        printf("value: \033[33;1m%%%u\033[0m io_state: \033[33;1m%%%u\033[0m ", (uint32_t)instr->return_value.value.value, (uint32_t)instr->return_value.io_state.value);
        break;
    case INSTR_IO_STATE:
        printf("producer: \033[33;1m%%%u\033[0m ", (uint32_t)instr->io_state.producer.value);
        break;
    case INSTR_REGION:
        printf("id: %u last_instr: \033[33;1m%%%u\033[0m ", (uint32_t)instr->region.id, (uint32_t)instr->region.last_instr.value);
        break;
    case INSTR_PHI:
        printf("variants: %.*s ", STR_FMT(instr_format_input_instrs(input_instr_buffer, instr->phi.variants, temp_allocator)));
        break;
    case INSTR_SELECT:
        printf("value: \033[33;1m%%%u\033[0m region: \033[33;1m%%%u\033[0m ", (uint32_t)instr->select.value.value, (uint32_t)instr->select.region.value);
        break;
    case INSTR_CALL_INTERNAL:
        printf("args: %.*s io_state: \033[33;1m%%%u\033[0m function_index: %u ", STR_FMT(instr_format_input_instrs(input_instr_buffer, instr->call_internal.args, temp_allocator)), (uint32_t)instr->call_internal.io_state.value, (uint32_t)instr->call_internal.function_index);
        break;
    case INSTR_COUNT:
        unreachable();
    }
    printf("\n");
}
