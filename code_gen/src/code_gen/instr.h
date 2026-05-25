#ifndef INSTR_H
#define INSTR_H

#ifdef CODE_GENERATION_PASS
	#include "stdint.h"

	typedef void Arena;
	typedef struct {
		const char* string;
		size_t length;
	} String;

	#define assert(...)
	#define assert_msg(...)
#else
	#include "core/core.h"
#endif

typedef struct Instr Instr;
typedef struct InstrBuffer InstrBuffer;
typedef struct InstrUsageRange InstrUsageRange;
typedef struct InstrStack InstrStack;
typedef struct InstrQueue InstrQueue;

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

	INSTR_LOGICAL_SHIFT_LEFT_8,
	INSTR_LOGICAL_SHIFT_LEFT_16,
	INSTR_LOGICAL_SHIFT_LEFT_32,
	INSTR_LOGICAL_SHIFT_LEFT_64,

	INSTR_LOGICAL_SHIFT_RIGHT_8,
	INSTR_LOGICAL_SHIFT_RIGHT_16,
	INSTR_LOGICAL_SHIFT_RIGHT_32,
	INSTR_LOGICAL_SHIFT_RIGHT_64,

	INSTR_COMPARE_8,
	INSTR_COMPARE_16,
	INSTR_COMPARE_32,
	INSTR_COMPARE_64,

	INSTR_CAST_TO_8,
	INSTR_CAST_TO_16,
	INSTR_CAST_TO_32,
	INSTR_CAST_TO_64,

	INSTR_PTR_LOAD_8,
	INSTR_PTR_LOAD_16,
	INSTR_PTR_LOAD_32,
	INSTR_PTR_LOAD_64,

	INSTR_LOAD_ARG,

	INSTR_BRANCH,
	INSTR_JUMP,

	INSTR_RETURN_VALUE,

	INSTR_IO_STATE,
	INSTR_REGION,

	INSTR_PHI,
	INSTR_SELECT,

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
	INSTR_CMP_EQUAL,
	INSTR_CMP_NOT_EQUAL,
	INSTR_CMP_LESS,
	INSTR_CMP_GREATER,
} InstrCompareKind;

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

// All the input instructions are in a single continuous array of `InstrIndex`,
// and this struct is used as a compact view into that array.
typedef struct {
	uint16_t start;
	uint16_t count;
} InstrInputs;

#ifndef CODE_GENERATION_PASS
static const InstrIndex INVALID_INSTR_INDEX = (InstrIndex) { .value = UINT16_MAX };
#endif // CODE_GENERATION_PASS

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
			InstrIndex operand;
			uint8_t shift_count;
		} logical_shift;

		struct {
			InstrCompareKind kind;
			InstrIndex left;
			InstrIndex right;
		} compare;

		struct {
			InstrIndex value;
		} cast;

		struct {
			InstrIndex ptr;
		} ptr_load;

		struct {
			uint8_t index;
		} load_arg;

		struct {
			InstrIndex condition;
			InstrIndex true_region;
			InstrIndex false_region;
			InstrIndex io_state;
		} branch;

		struct {
			InstrIndex target_region;
			InstrIndex io_state;
		} jump;
		
		struct {
			InstrIndex value;
			InstrIndex io_state;
		} return_value;

		struct {
			InstrIndex producer;
		} io_state;

		struct {
			InstrInputs args;
			InstrIndex io_state;
			uint16_t function_index;
		} call_internal;

		struct {
			uint16_t id;
			InstrIndex last_instr;
		} region;

		struct {
			InstrInputs variants;
		} phi;

		struct {
			InstrIndex value;
			InstrIndex region;
		} select;
	};
};

#ifndef CODE_GENERATION_PASS

struct InstrBuffer {
	Instr* instr;
	InstrIndex* inputs_buffer;

	uint16_t count;
	uint16_t inputs_buffer_size;
	uint16_t region_count;
};

inline InstrInputs instr_allocate_inputs_array(InstrBuffer* buffer,
		uint16_t count,
		Arena* inputs_buffer_allocator) {

	const void* inputs_buffer_end = buffer->inputs_buffer + buffer->inputs_buffer_size;
	const void* arena_allocation_end = inputs_buffer_allocator->base + inputs_buffer_allocator->allocated;
	assert_msg(inputs_buffer_end == arena_allocation_end,
			"The arena was ment to be exclusively used for allocating arrays of input instructions");

	arena_alloc_array(inputs_buffer_allocator, uint16_t, count);

	InstrInputs inputs = {};
	inputs.start = buffer->inputs_buffer_size;
	inputs.count = count;

	buffer->inputs_buffer_size += count;
	return inputs;
}

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

struct InstrQueue {
	InstrIndex* buffer;
	size_t head;
	size_t count;
	size_t capacity;
};

inline void instr_queue_alloc(InstrQueue* queue, Arena* allocator, size_t capacity) {
	assert(capacity >= 1);

	queue->buffer = arena_alloc_array(allocator, InstrIndex, capacity);
	queue->head = 0;
	queue->count = 0;
	queue->capacity = capacity;
}

inline void instr_queue_push_back(InstrQueue* queue, InstrIndex instr) {
	assert_msg(queue->count != queue->capacity, "Queue is full");

	size_t insert_index = (queue->head + queue->count) % queue->capacity;
	queue->buffer[insert_index] = instr;
	queue->count += 1;
}

inline InstrIndex instr_queue_pop_front(InstrQueue* queue) {
	assert(queue->count > 0);

	InstrIndex instr = queue->buffer[queue->head];
	queue->head = (queue->head + 1) % queue->capacity;
	queue->count -= 1;
	return instr;
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
	instr->region.id = buffer->region_count;
	buffer->region_count += 1;
	return i;
}

inline InstrIndex instr_new_jump(InstrBuffer* buffer, Arena* allocator, InstrIndex target, InstrIndex io_state) {
	InstrIndex i = instr_buffer_append(buffer, allocator);
	Instr* instr = instr_buffer_at(buffer, i);
	instr->kind = INSTR_JUMP;
	instr->jump.target_region = target;
	instr->jump.io_state = io_state;
	return i;
}

inline InstrIndex instr_new_return_value(InstrBuffer* buffer, Arena* allocator, InstrIndex value, InstrIndex io_state) {
	const Instr* io_state_instr = instr_buffer_at(buffer, io_state);
	assert(io_state_instr->kind == INSTR_IO_STATE);

	InstrIndex i = instr_buffer_append(buffer, allocator);
	Instr* instr = instr_buffer_at(buffer, i);
	instr->kind = INSTR_RETURN_VALUE;
	instr->return_value.value = value;
	instr->return_value.io_state = io_state;
	return i;
}

inline InstrIndex instr_new_io_state(InstrBuffer* buffer, Arena* allocator, InstrIndex producer) {
	InstrIndex i = instr_buffer_append(buffer, allocator);
	Instr* instr = instr_buffer_at(buffer, i);
	instr->kind = INSTR_IO_STATE;
	instr->io_state.producer = producer;
	return i;
}

inline InstrIndex instr_new_logical_shift_left_by(InstrBuffer* buffer,
		Arena* allocator,
		InstrIndex operand,
		uint8_t shift_count) {

	InstrIndex shift_index = instr_buffer_append(buffer, allocator);
	Instr* shift_instr = instr_buffer_at(buffer, shift_index);
	shift_instr->kind = INSTR_LOGICAL_SHIFT_LEFT_64;
	shift_instr->logical_shift.operand = operand;
	shift_instr->logical_shift.shift_count = shift_count;
	return shift_index;
}

// Creates a INSTR_CAST_<target_bit_count>
InstrIndex instr_new_cast(InstrBuffer* buffer,
		Arena* allocator,
		InstrIndex value,
		uint8_t target_bit_count);

uint16_t instr_region_id(const InstrBuffer* buffer, InstrIndex region_index);
bool instr_region_finished(const InstrBuffer* buffer, InstrIndex region_index);

void instr_push_input_dependeices(const InstrBuffer* buffer, InstrInputs inputs, InstrStack* out_dependencies);
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

InstrIndexArray _x64_gather_regions_in_bfs_order(const InstrBuffer instr_buffer,
		Arena* allocator,
		Arena* temp_allocator,
		InstrIndex start_region);

String instr_name(InstrKind instr_kind);
String instr_bin_op_name(InstrBinOp op_kind);
String instr_compare_kind_name(InstrCompareKind kind);

String instr_format_input_instrs(const InstrIndex* input_instr_buffer,
		InstrInputs inputs,
		Arena* temp_allocator);

void instr_print(const Instr* instr, const InstrIndex* input_instr_buffer, Arena* temp_allocator);
void instr_print_all(InstrBuffer instr_buffer, Arena* temp_allocator);

// Turns unused instruciton into INSTR_NO_OP.
// Doesn't removed these instructions from the array.
void instr_replace_dead_instr(const InstrBuffer instr_buffer, const InstrUsageRange* usage_ranges);

#endif // CODE_GENERATION_PASS

#endif
