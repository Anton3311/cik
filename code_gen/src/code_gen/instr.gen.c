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
String instr_name(InstrKind instr_kind) {
	return s_instr_kind_to_string[instr_kind];
}
String instr_bin_op_name(InstrBinOp op_kind) {
	return s_instr_bin_op_kind_to_string[op_kind];
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
