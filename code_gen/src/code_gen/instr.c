#include "instr.h"

static String instr_kind_to_string[] = {
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
	[INSTR_REGION] = STR_LIT("region"),
};

static String s_instr_bin_op_kind_to_string[] = {
	[INSTR_BIN_ADD] = STR_LIT("add"),
	[INSTR_BIN_SUB] = STR_LIT("sub"),
	[INSTR_BIN_MUL] = STR_LIT("mul"),
	[INSTR_BIN_DIV] = STR_LIT("div"),
};

String instr_name(InstrKind instr_kind) {
	return instr_kind_to_string[instr_kind];
}

void instr_print(const Instr* instr) {
	String name = instr_name(instr->kind);

	size_t name_width = 12;

	printf("\033[32;1m%.*s\033[0m \033[%uC", STR_FMT(name), (uint32_t)(name_width - name.length));

	switch (instr->kind) {
	case INSTR_NO_OP:
		break;

	case INSTR_CONST_8:
		printf("%u %d", (uint32_t)instr->const_8.u8, (int32_t)instr->const_8.i8);
		break;
	case INSTR_CONST_16:
		printf("%u %d", (uint32_t)instr->const_16.u16, (int32_t)instr->const_16.i16);
		break;
	case INSTR_CONST_32:
		printf("%u %d %f",
				instr->const_32.u32,
				instr->const_32.i32,
				instr->const_32.f32);
		break;
	case INSTR_CONST_64:
		printf("%llu %lld %f",
				instr->const_64.i64,
				instr->const_64.u64,
				instr->const_64.f64);
		break;
	case INSTR_BRANCH:
		printf("condition: %u true: %u false: %u",
				(uint32_t)instr->branch.condition.value,
				(uint32_t)instr->branch.true_region.value,
				(uint32_t)instr->branch.false_region.value);
		break;
	case INSTR_JUMP:
		printf("%u", (uint32_t)instr->jump.target_region.value);
		break;
	case INSTR_REGION:
		printf("%u", (uint32_t)instr->region.last_instr.value);
		break;
	case INSTR_BIN_OP_8:
	case INSTR_BIN_OP_16:
	case INSTR_BIN_OP_32:
	case INSTR_BIN_OP_64: {
		printf("op: %.*s left: %u right: %u",
				STR_FMT(s_instr_bin_op_kind_to_string[instr->bin_op.kind]),
				(uint32_t)instr->bin_op.left.value,
				(uint32_t)instr->bin_op.right.value);
		break;
	}
	}

	printf("\n");
}

void instr_print_all(InstrBuffer instr_buffer) {
	for (size_t i = 0; i < instr_buffer.count; i += 1) {
		printf("%zu\033[12G", i);
		instr_print(&instr_buffer.instr[i]);
	}
}
