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
    case INSTR_UNINITIALIZED_8: return STR_LIT("uninitialized_8");
    case INSTR_UNINITIALIZED_16: return STR_LIT("uninitialized_16");
    case INSTR_UNINITIALIZED_32: return STR_LIT("uninitialized_32");
    case INSTR_UNINITIALIZED_64: return STR_LIT("uninitialized_64");
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
    case INSTR_BITWISE_NOT_8: return STR_LIT("bitwise_not_8");
    case INSTR_BITWISE_NOT_16: return STR_LIT("bitwise_not_16");
    case INSTR_BITWISE_NOT_32: return STR_LIT("bitwise_not_32");
    case INSTR_BITWISE_NOT_64: return STR_LIT("bitwise_not_64");
    case INSTR_NOT: return STR_LIT("not");
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
    case INSTR_MEM_COPY_FIXED: return STR_LIT("mem_copy_fixed");
    case INSTR_LOAD_ARG_8: return STR_LIT("load_arg_8");
    case INSTR_LOAD_ARG_16: return STR_LIT("load_arg_16");
    case INSTR_LOAD_ARG_32: return STR_LIT("load_arg_32");
    case INSTR_LOAD_ARG_64: return STR_LIT("load_arg_64");
    case INSTR_STACK_ALLOC: return STR_LIT("stack_alloc");
    case INSTR_STACK_ADDR: return STR_LIT("stack_addr");
    case INSTR_BRANCH: return STR_LIT("branch");
    case INSTR_JUMP: return STR_LIT("jump");
    case INSTR_RET: return STR_LIT("ret");
    case INSTR_RETURN_VALUE: return STR_LIT("return_value");
    case INSTR_IO_STATE: return STR_LIT("io_state");
    case INSTR_REGION: return STR_LIT("region");
    case INSTR_PHI: return STR_LIT("phi");
    case INSTR_SELECT: return STR_LIT("select");
    case INSTR_CALL_INDIRECT: return STR_LIT("call_indirect");
    case INSTR_CALL_DIRECT: return STR_LIT("call_direct");
    case INSTR_COUNT: unreachable();
    }
    unreachable();
    return (String) {};
}
String instr_bin_op_name(InstrBinOp variant) {
    switch (variant) {
    case INSTR_BIN_ADD: return STR_LIT("add");
    case INSTR_BIN_SUB: return STR_LIT("sub");
    case INSTR_BIN_IMUL: return STR_LIT("imul");
    case INSTR_BIN_IDIV: return STR_LIT("idiv");
    case INSTR_BIN_IMOD: return STR_LIT("imod");
    case INSTR_BIN_UMUL: return STR_LIT("umul");
    case INSTR_BIN_UDIV: return STR_LIT("udiv");
    case INSTR_BIN_UMOD: return STR_LIT("umod");
    case INSTR_BIN_AND: return STR_LIT("and");
    case INSTR_BIN_OR: return STR_LIT("or");
    case INSTR_BIN_XOR: return STR_LIT("xor");
    case INSTR_BIN_SHIFT_LEFT: return STR_LIT("shift_left");
    case INSTR_BIN_SHIFT_RIGHT: return STR_LIT("shift_right");
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
    case INSTR_UNINITIALIZED_8:
        break;
    case INSTR_UNINITIALIZED_16:
        break;
    case INSTR_UNINITIALIZED_32:
        break;
    case INSTR_UNINITIALIZED_64:
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
    case INSTR_BITWISE_NOT_8:
        instr_queue_push_back(out_dependencies, instr->bitwise_not.operand);
        break;
    case INSTR_BITWISE_NOT_16:
        instr_queue_push_back(out_dependencies, instr->bitwise_not.operand);
        break;
    case INSTR_BITWISE_NOT_32:
        instr_queue_push_back(out_dependencies, instr->bitwise_not.operand);
        break;
    case INSTR_BITWISE_NOT_64:
        instr_queue_push_back(out_dependencies, instr->bitwise_not.operand);
        break;
    case INSTR_NOT:
        instr_queue_push_back(out_dependencies, instr->not.operand);
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
    case INSTR_MEM_COPY_FIXED:
        instr_queue_push_back(out_dependencies, instr->mem_copy_fixed.src);
        instr_queue_push_back(out_dependencies, instr->mem_copy_fixed.dst);
        instr_queue_push_back(out_dependencies, instr->mem_copy_fixed.io_state);
        break;
    case INSTR_LOAD_ARG_8:
        break;
    case INSTR_LOAD_ARG_16:
        break;
    case INSTR_LOAD_ARG_32:
        break;
    case INSTR_LOAD_ARG_64:
        break;
    case INSTR_STACK_ALLOC:
        break;
    case INSTR_STACK_ADDR:
        instr_queue_push_back(out_dependencies, instr->stack_addr.stack_alloc);
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
    case INSTR_CALL_INDIRECT:
        instr_push_input_dependencies(buffer, instr->call.args, out_dependencies);
        instr_queue_push_back(out_dependencies, instr->call.io_state);
        break;
    case INSTR_CALL_DIRECT:
        instr_push_input_dependencies(buffer, instr->call.args, out_dependencies);
        instr_queue_push_back(out_dependencies, instr->call.io_state);
        break;
    case INSTR_COUNT:
        unreachable();
    }
}
void instr_print(const Instr* instr, const InstrIndex* input_instr_buffer, Arena* temp_allocator) {
    String name = instr_name(instr->kind);

    size_t name_width = 17;

    printf("\033[32;1m%.*s\033[0m \033[%uC", STR_FMT(name), (uint32_t)(name_width - name.length));

    switch (instr->kind) {
    case INSTR_NO_OP:
        break;
    case INSTR_UNINITIALIZED_8:
        break;
    case INSTR_UNINITIALIZED_16:
        break;
    case INSTR_UNINITIALIZED_32:
        break;
    case INSTR_UNINITIALIZED_64:
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
    case INSTR_BITWISE_NOT_8:
        printf("operand: \033[33;1m%%%u\033[0m ", (uint32_t)instr->bitwise_not.operand.value);
        break;
    case INSTR_BITWISE_NOT_16:
        printf("operand: \033[33;1m%%%u\033[0m ", (uint32_t)instr->bitwise_not.operand.value);
        break;
    case INSTR_BITWISE_NOT_32:
        printf("operand: \033[33;1m%%%u\033[0m ", (uint32_t)instr->bitwise_not.operand.value);
        break;
    case INSTR_BITWISE_NOT_64:
        printf("operand: \033[33;1m%%%u\033[0m ", (uint32_t)instr->bitwise_not.operand.value);
        break;
    case INSTR_NOT:
        printf("operand: \033[33;1m%%%u\033[0m ", (uint32_t)instr->not.operand.value);
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
    case INSTR_MEM_COPY_FIXED:
        printf("src: \033[33;1m%%%u\033[0m dst: \033[33;1m%%%u\033[0m io_state: \033[33;1m%%%u\033[0m size: %u ", (uint32_t)instr->mem_copy_fixed.src.value, (uint32_t)instr->mem_copy_fixed.dst.value, (uint32_t)instr->mem_copy_fixed.io_state.value, (uint32_t)instr->mem_copy_fixed.size);
        break;
    case INSTR_LOAD_ARG_8:
        printf("index: %u ", (uint32_t)instr->load_arg.index);
        break;
    case INSTR_LOAD_ARG_16:
        printf("index: %u ", (uint32_t)instr->load_arg.index);
        break;
    case INSTR_LOAD_ARG_32:
        printf("index: %u ", (uint32_t)instr->load_arg.index);
        break;
    case INSTR_LOAD_ARG_64:
        printf("index: %u ", (uint32_t)instr->load_arg.index);
        break;
    case INSTR_STACK_ALLOC:
        printf("size: %u alignment: %u ", (uint32_t)instr->stack_alloc.size, (uint32_t)instr->stack_alloc.alignment);
        break;
    case INSTR_STACK_ADDR:
        printf("stack_alloc: \033[33;1m%%%u\033[0m ", (uint32_t)instr->stack_addr.stack_alloc.value);
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
    case INSTR_CALL_INDIRECT:
        printf("args: %.*s io_state: \033[33;1m%%%u\033[0m function_index: %u ", STR_FMT(instr_format_input_instrs(input_instr_buffer, instr->call.args, temp_allocator)), (uint32_t)instr->call.io_state.value, (uint32_t)instr->call.function_index);
        break;
    case INSTR_CALL_DIRECT:
        printf("args: %.*s io_state: \033[33;1m%%%u\033[0m function_index: %u ", STR_FMT(instr_format_input_instrs(input_instr_buffer, instr->call.args, temp_allocator)), (uint32_t)instr->call.io_state.value, (uint32_t)instr->call.function_index);
        break;
    case INSTR_COUNT:
        unreachable();
    }
    printf("\n");
}
