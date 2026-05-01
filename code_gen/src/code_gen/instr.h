#ifndef INSTR_H
#define INSTR_H

#include "core/core.h"

typedef struct Instr Instr;
typedef struct InstrBuffer InstrBuffer;
typedef struct InstrUsageRange InstrUsageRange;
typedef struct InstrStack InstrStack;

typedef enum {
	INSTR_NO_OP,

	INSTR_CONST_8,
	INSTR_CONST_16,
	INSTR_CONST_32,
	INSTR_CONST_64,

	INSTR_BIN_OP_8,
	INSTR_BIN_OP_16,
	INSTR_BIN_OP_32,
	INSTR_BIN_OP_64,

	INSTR_BRANCH,
	INSTR_JUMP,

	INSTR_RETURN_VALUE,

	INSTR_IO_STATE,
	INSTR_REGION,

	INSTR_CALL_INTERNAL,

	INSTR_COUNT,
} InstrKind;

typedef enum {
	INSTR_BIN_ADD,
	INSTR_BIN_SUB,
	INSTR_BIN_MUL,
	INSTR_BIN_DIV,
} InstrBinOp;

typedef enum {
	INSTR_FEATURE_NONE                 = 0,
	INSTR_FEATURE_CONTROL              = 1 << 0,
	INSTR_FEATURE_REG_STORAGE          = 1 << 1,
} InstrFeatureFlag;

extern InstrFeatureFlag INSTR_FEATURES[INSTR_COUNT];

typedef struct {
	uint16_t value;
} InstrIndex;

typedef struct {
	InstrIndex* instr;
	size_t count;
} InstrIndexArray;

static const InstrIndex INVALID_INSTR_INDEX = (InstrIndex) { .value = UINT16_MAX };

struct Instr {
	InstrKind kind;

	union {
		union {
			uint8_t u;
			int8_t i;
		} const_8;

		union {
			uint16_t u;
			int16_t i;
		} const_16;

		union {
			uint32_t u;
			int32_t i;
			float f;
		} const_32;

		union {
			uint64_t u;
			int64_t i;
			double f;
		} const_64;

		// The same for all INSTR_BIN_OP_*
		struct {
			InstrBinOp kind;
			InstrIndex left;
			InstrIndex right;
		} bin_op;

		struct {
			InstrIndex condition;
			InstrIndex true_region;
			InstrIndex false_region;
		} branch;

		struct {
			InstrIndex target_region;
		} jump;
		
		struct {
			InstrIndex value;
		} ret;

		struct {
			InstrIndex producer;
		} io_state;

		struct {
			InstrIndex arg;
			InstrIndex io_state;
			uint8_t function_index;
		} call_internal;

		struct {
			InstrIndex last_instr;
			InstrIndex io_state;
		} region;
	};
};

struct InstrBuffer {
	Instr* instr;
	uint16_t count;
};

struct InstrStack {
	InstrIndex* instr;
	size_t count;
	size_t capacity;
};

inline void instr_stack_alloc(InstrStack* stack, Arena* allocator, size_t capacity) {
	stack->count = 0;
	stack->capacity = capacity;
	stack->instr = arena_alloc_array(allocator, InstrIndex, capacity);
}

inline void instr_stack_push(InstrStack* stack, InstrIndex instr) {
	assert(stack->count < stack->capacity);
	stack->instr[stack->count] = instr;
	stack->count += 1;
}

inline InstrIndex instr_stack_pop(InstrStack* stack) {
	assert(stack->count > 0);
	stack->count -= 1;
	return stack->instr[stack->count];
}

struct InstrUsageRange {
	union {
		struct {
			InstrIndex first_usage;
			InstrIndex last_usage;
		};

		uint32_t value;
	};
};

inline bool instr_usage_range_is_valid(const InstrUsageRange range) {
	return range.value != UINT32_MAX && range.first_usage.value <= range.last_usage.value;
}

inline bool instr_usage_range_is_empty(const InstrUsageRange range) {
	return range.value == UINT32_MAX;
}

// Returns a new range that includes the given instruction index
inline InstrUsageRange instr_usage_range_extended(const InstrUsageRange range, InstrIndex instr_index) {
	InstrUsageRange new_range;
	new_range.first_usage.value = min(range.first_usage.value, instr_index.value);
	new_range.last_usage.value = max(range.last_usage.value, instr_index.value);
	return new_range;
}

inline void instr_buffer_init(InstrBuffer* buffer, Arena* allocator) {
	buffer->instr = arena_alloc_array(allocator, Instr, 0);
	buffer->count = 0;
}

inline InstrIndex instr_buffer_append(InstrBuffer* buffer, Arena* allocator) {
	assert(buffer->count <= UINT16_MAX);

	Instr* instr = arena_alloc(allocator, Instr);
	(void)instr;

	InstrIndex i = { .value = buffer->count };
	buffer->count += 1;
	return i;
}

#define instr_buffer_at(instr_buffer, index) &instr_buffer->instr[index.value]

inline InstrIndex instr_new_region(InstrBuffer* buffer, Arena* allocator) {
	InstrIndex i = instr_buffer_append(buffer, allocator);
	Instr* instr = instr_buffer_at(buffer, i);
	instr->kind = INSTR_REGION;
	instr->region.last_instr.value = UINT16_MAX;
	return i;
}

inline InstrIndex instr_new_jump(InstrBuffer* buffer, Arena* allocator, InstrIndex target) {
	InstrIndex i = instr_buffer_append(buffer, allocator);
	Instr* instr = instr_buffer_at(buffer, i);
	instr->kind = INSTR_JUMP;
	instr->jump.target_region = target;
	return i;
}

inline InstrIndex instr_new_return_value(InstrBuffer* buffer, Arena* allocator, InstrIndex value) {
	InstrIndex i = instr_buffer_append(buffer, allocator);
	Instr* instr = instr_buffer_at(buffer, i);
	instr->kind = INSTR_RETURN_VALUE;
	instr->ret.value = value;
	return i;
}

inline InstrIndex instr_new_io_state(InstrBuffer* buffer, Arena* allocator, InstrIndex producer) {
	InstrIndex i = instr_buffer_append(buffer, allocator);
	Instr* instr = instr_buffer_at(buffer, i);
	instr->kind = INSTR_IO_STATE;
	instr->io_state.producer = producer;
	return i;
}

bool instr_region_finished(const InstrBuffer* buffer, InstrIndex region_index);

void instr_enumerate_dependencies(const InstrBuffer buffer, InstrIndex instr_index, InstrStack* out_dependencies);

typedef struct {
	InstrIndex* instr;
	size_t count;
} InstrUsageGroup;

typedef struct {
	InstrUsageGroup* groups;
	size_t count;
} InstrUsageGroupArray;

InstrUsageGroupArray instr_group_by_overlapping_usages(const InstrBuffer buffer,
		const InstrUsageRange* usage_ranges,
		Arena* allocator,
		Arena* temp_allocator);

// Returns an array of `InstrUsageRange` of size `buffer->count`
InstrUsageRange* instr_compute_usage_ranges(const InstrBuffer buffer,
		InstrIndex root_instr,
		Arena* allocator,
		Arena* temp_allocator);

String instr_name(InstrKind instr_kind);
void instr_print(const Instr* instr);
void instr_print_all(InstrBuffer instr_buffer);

#endif
