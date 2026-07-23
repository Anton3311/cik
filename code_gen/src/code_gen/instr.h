#ifndef INSTR_H
#define INSTR_H

#ifdef CODE_GENERATION_PASS
	#include "stdint.h"

	typedef void Arena;
	typedef struct {
		const char* string;
		size_t length;
	} String;

	typedef uint8_t bool;

	#define assert(...)
	#define assert_msg(...)
#else
	#include "core/core.h"
#endif

typedef struct Instr Instr;
typedef struct InstrBuffer InstrBuffer;
typedef union InstrLiveRange InstrLiveRange;
typedef struct InstrQueue InstrQueue;

typedef enum {
	INSTR_NO_OP,

	// An uninitialized value
	//
	// Doesn't have a correponding veriant in the `Instr` struct
	INSTR_UNINITIALIZED_8,
	INSTR_UNINITIALIZED_16,
	INSTR_UNINITIALIZED_32,
	INSTR_UNINITIALIZED_64,

	INSTR_CONST_8,
	INSTR_CONST_16,
	INSTR_CONST_32,
	INSTR_CONST_64,

	INSTR_CONST_STRING,

	INSTR_BIN_OP_8,
	INSTR_BIN_OP_16,
	INSTR_BIN_OP_32,
	INSTR_BIN_OP_64,

	INSTR_NEGATE_8,
	INSTR_NEGATE_16,
	INSTR_NEGATE_32,
	INSTR_NEGATE_64,

	INSTR_BITWISE_NOT_8,
	INSTR_BITWISE_NOT_16,
	INSTR_BITWISE_NOT_32,
	INSTR_BITWISE_NOT_64,

	// A boolean not
	INSTR_NOT,

	INSTR_COMPARE_8,
	INSTR_COMPARE_16,
	INSTR_COMPARE_32,
	INSTR_COMPARE_64,

	INSTR_BOOL_TO_INT,

	INSTR_CAST_TO_8,
	INSTR_CAST_TO_16,
	INSTR_CAST_TO_32,
	INSTR_CAST_TO_64,

	INSTR_PTR_LOAD_8,
	INSTR_PTR_LOAD_16,
	INSTR_PTR_LOAD_32,
	INSTR_PTR_LOAD_64,

	INSTR_PTR_STORE_8,
	INSTR_PTR_STORE_16,
	INSTR_PTR_STORE_32,
	INSTR_PTR_STORE_64,

	INSTR_LOAD_ARG_8,
	INSTR_LOAD_ARG_16,
	INSTR_LOAD_ARG_32,
	INSTR_LOAD_ARG_64,

	INSTR_BRANCH,
	INSTR_JUMP,

	INSTR_RET,
	INSTR_RETURN_VALUE,

	INSTR_IO_STATE,
	INSTR_REGION,

	INSTR_PHI,
	INSTR_SELECT,

	INSTR_CALL_INDIRECT,

	INSTR_COUNT,
} InstrKind;

typedef enum {
	INSTR_BIN_ADD,
	INSTR_BIN_SUB,
	INSTR_BIN_IMUL,
	INSTR_BIN_IDIV,
	INSTR_BIN_IMOD,
	INSTR_BIN_UMUL,
	INSTR_BIN_UDIV,
	INSTR_BIN_UMOD,

	INSTR_BIN_AND,
	INSTR_BIN_OR,
	INSTR_BIN_XOR,

	INSTR_BIN_SHIFT_LEFT,
	INSTR_BIN_SHIFT_RIGHT,
} InstrBinOp;

bool instr_bin_op_is_commutative(InstrBinOp op);

typedef enum {
	INSTR_CMP_EQUAL,
	INSTR_CMP_NOT_EQUAL,
	INSTR_CMP_LESS,
	INSTR_CMP_LESS_OR_EQUAL,
	INSTR_CMP_GREATER,
	INSTR_CMP_GREATER_OR_EQUAL,
} InstrCompareKind;

InstrCompareKind instr_compare_kind_flip(InstrCompareKind kind);

typedef enum {
	INSTR_FEATURE_NONE                 = 0,
	INSTR_FEATURE_CONTROL              = 1 << 0,
	INSTR_FEATURE_REG_STORAGE          = 1 << 1,
	INSTR_FEATURE_BOOL                 = 1 << 2,
} InstrFeatureFlag;

extern InstrFeatureFlag INSTR_FEATURES[INSTR_COUNT];

typedef struct {
	uint16_t value;
} InstrIndex;

#define INVALID_INSTR_INDEX (InstrIndex) { UINT16_MAX }

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

		struct {
			uint32_t string_id;
		} const_string;

		// The same for all INSTR_BIN_OP_*
		struct {
			InstrBinOp kind;
			InstrIndex left;
			InstrIndex right;
		} bin_op;

		struct {
			InstrIndex operand;
		} negate;

		struct {
			InstrIndex operand;
		} bitwise_not;

		struct {
			InstrIndex operand;
		} not;

		struct {
			InstrCompareKind kind;
			InstrIndex left;
			InstrIndex right;
		} compare;

		struct {
			InstrIndex operand;
		} bool_to_int;

		struct {
			InstrIndex value;
		} cast;

		struct {
			InstrIndex ptr;
			InstrIndex io_state;
		} ptr_load;

		struct {
			InstrIndex ptr;
			InstrIndex value;
			InstrIndex io_state;
		} ptr_store;

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

		// Just returns without the value
		struct {
			InstrIndex io_state;
		} ret;

		struct {
			InstrIndex producer;
		} io_state;

		struct {
			InstrInputs args;
			InstrIndex io_state;
			uint16_t function_index;
		} call_indirect;

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

//
// InstrBuffer
//

struct InstrBuffer {
	Instr* instr;
	InstrIndex* inputs_buffer;

	uint16_t count;
	uint16_t inputs_buffer_size;
	uint16_t inputs_buffer_capacity;
	uint16_t region_count;
};

#define instr_buffer_at(instr_buffer, index) &instr_buffer->instr[(index).value]

void instr_buffer_init(InstrBuffer* buffer, Arena* allocator);
void instr_buffer_release(InstrBuffer* buffer);

InstrInputs instr_allocate_inputs_array(InstrBuffer* buffer, uint16_t count);

inline InstrIndex instr_buffer_append(InstrBuffer* buffer, Arena* allocator) {
	assert(buffer->count <= UINT16_MAX);

	const uint8_t* arena_end = (const uint8_t*)allocator->base + allocator->allocated;
	const Instr* buffer_end = buffer->instr + buffer->count;
	assert((const void*)arena_end == (const void*)buffer_end);

	Instr* instr = arena_alloc(allocator, Instr);
	memset(instr, 0xff, sizeof(*instr));

	InstrIndex i = { .value = buffer->count };
	buffer->count += 1;
	return i;
}

//
// InstrQueue
//

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

inline void instr_queue_init(InstrQueue* queue, InstrIndex* backing_buffer, size_t capacity) {
	assert(capacity >= 1);

	queue->buffer = backing_buffer;
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

inline InstrIndex instr_queue_pop_back(InstrQueue* queue) {
	assert(queue->count > 0);

	InstrIndex instr = queue->buffer[(queue->head + queue->count - 1) % queue->capacity];
	queue->count -= 1;
	return instr;
}

inline bool instr_is_control(const InstrBuffer* instr_buffer, InstrIndex instr_index) {
	const Instr* instr = instr_buffer_at(instr_buffer, instr_index);
	return has_flag(INSTR_FEATURES[instr->kind], INSTR_FEATURE_CONTROL);
}

// `int_size` - size of the int in bytes
InstrIndex instr_new_int_const(InstrBuffer* buffer,
		Arena* allocator,
		uint64_t value,
		size_t int_size);

inline InstrIndex instr_new_region(InstrBuffer* buffer, Arena* allocator) {
	InstrIndex i = instr_buffer_append(buffer, allocator);
	Instr* instr = instr_buffer_at(buffer, i);
	instr->kind = INSTR_REGION;
	instr->region.last_instr.value = UINT16_MAX;
	instr->region.id = buffer->region_count;
	buffer->region_count += 1;
	return i;
}

inline void instr_region_set_last(InstrBuffer* buffer,
		InstrIndex region_index,
		InstrIndex control_instr_index) {
	assert(instr_is_control(buffer, control_instr_index));

	Instr* region = instr_buffer_at(buffer, region_index);
	assert(region->kind == INSTR_REGION);
	
	region->region.last_instr = control_instr_index;
}

// Creates a new `INSTR_JUMP` instruction
//
// Consumes the io state from `io_state`, and writes the new one back to `io_state`.
InstrIndex instr_new_jump(InstrBuffer* buffer,
		Arena* allocator,
		InstrIndex target,
		InstrIndex* io_state);

inline void instr_set_jump_target(InstrBuffer* buffer, InstrIndex jump, InstrIndex target) {
	Instr* instr = instr_buffer_at(buffer, jump);
	instr->jump.target_region = target;
}

// Creates a new `INSTR_RETURN_VALUE` instruction
//
// Consumes the io state from `io_state`.
//
// NOTE: Doens't create a new `io_state`
InstrIndex instr_new_return_value(InstrBuffer* buffer,
		Arena* allocator,
		InstrIndex value,
		InstrIndex* io_state);

// Creates a new `INSTR_RET` instruction
//
// Consumes the io state from `io_state`.
//
// NOTE: Doens't create a new `io_state`
InstrIndex instr_new_return(InstrBuffer* buffer,
		Arena* allocator,
		InstrIndex* io_state);

// Creates an empty INSTR_PHI without any variants.
//
// Doesn't even reserve the array for variants.
InstrIndex instr_new_empty_phi(InstrBuffer* buffer, Arena* allocator);

inline InstrIndex instr_new_io_state(InstrBuffer* buffer, Arena* allocator, InstrIndex producer) {
	InstrIndex i = instr_buffer_append(buffer, allocator);
	Instr* instr = instr_buffer_at(buffer, i);
	instr->kind = INSTR_IO_STATE;
	instr->io_state.producer = producer;
	return i;
}

// `operand_size` - size of an operand in bytes
InstrIndex instr_new_logical_shift_left_by(InstrBuffer* buffer,
		Arena* allocator,
		InstrIndex operand,
		uint8_t operand_size,
		uint8_t shift_count);

// Creates a INSTR_CAST_<target_bit_count>
InstrIndex instr_new_cast(InstrBuffer* buffer,
		Arena* allocator,
		InstrIndex value,
		uint8_t target_bit_count);

uint16_t instr_region_id(const InstrBuffer* buffer, InstrIndex region_index);
bool instr_region_finished(const InstrBuffer* buffer, InstrIndex region_index);

void instr_push_input_dependencies(const InstrBuffer* buffer,
		InstrInputs inputs,
		InstrQueue* out_dependencies);

// Pushes all the uses (inputs) of this instructions to the back of the queue
void instr_enumerate_uses(const InstrBuffer* buffer,
		InstrIndex instr_index,
		InstrQueue* queue);

union InstrLiveRange {
	struct {
		InstrIndex start;
		InstrIndex end;
	};

	uint32_t value;
};

// Returns an array of `InstrLiveRange` of size `buffer->count`
InstrLiveRange* instr_compute_live_ranges(const InstrBuffer buffer,
		InstrIndex root_instr,
		Arena* allocator,
		Arena* temp_allocator);

InstrIndexArray _instr_gather_regions_in_dfs_order(const InstrBuffer instr_buffer,
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

#endif // CODE_GENERATION_PASS

#endif
