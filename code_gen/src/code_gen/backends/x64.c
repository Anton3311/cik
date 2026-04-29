#include "x64.h"

#include "core/profiler.h"

static X64InstrStorageRequirement s_instr_storage_requiremenets[INSTR_COUNT] = {
	[INSTR_NO_OP]        = (X64InstrStorageRequirement) { .allowed_registers = 0, .reg_size = 0 },

	[INSTR_CONST_8]      = (X64InstrStorageRequirement) { .allowed_registers = UINT16_MAX, .reg_size = 8 },
	[INSTR_CONST_16]     = (X64InstrStorageRequirement) { .allowed_registers = UINT16_MAX, .reg_size = 16 },
	[INSTR_CONST_32]     = (X64InstrStorageRequirement) { .allowed_registers = UINT16_MAX, .reg_size = 32 },
	[INSTR_CONST_64]     = (X64InstrStorageRequirement) { .allowed_registers = UINT16_MAX, .reg_size = 64 },

	[INSTR_BIN_OP_8]     = (X64InstrStorageRequirement) { .allowed_registers = UINT16_MAX, .reg_size = 8 },
	[INSTR_BIN_OP_16]    = (X64InstrStorageRequirement) { .allowed_registers = UINT16_MAX, .reg_size = 16 },
	[INSTR_BIN_OP_32]    = (X64InstrStorageRequirement) { .allowed_registers = UINT16_MAX, .reg_size = 32 },
	[INSTR_BIN_OP_64]    = (X64InstrStorageRequirement) { .allowed_registers = UINT16_MAX, .reg_size = 64 },

	[INSTR_BRANCH]       = (X64InstrStorageRequirement) { .allowed_registers = 0, .reg_size = 0 },
	[INSTR_JUMP]         = (X64InstrStorageRequirement) { .allowed_registers = 0, .reg_size = 0 },

	[INSTR_RETURN_VALUE] = (X64InstrStorageRequirement) { .allowed_registers = 0, .reg_size = 0 },

	[INSTR_CALL_INTERNAL] = (X64InstrStorageRequirement) { .allowed_registers = UINT16_MAX, .reg_size = 64 },

	[INSTR_REGION]       = (X64InstrStorageRequirement) { .allowed_registers = 0, .reg_size = 0 },
};

static const char* REG_BASE_NAMES[] = {
	"A",
	"C",
	"D",
	"B",
	"SP",
	"BP",
	"SI",
	"DI",
	"R8",
	"R9",
	"R10",
	"R11",
	"R12",
	"R13",
	"R14",
	"R15",
};

static void _format_reg_name(StringBuilder* builder, uint16_t reg_index, uint8_t reg_bit_count) {
	assert(reg_index < 16);

	char name_prefix = 0;
	char name_sufix = 0;

	switch (reg_bit_count) {
	case 8:
		name_sufix = 'L';
		break;
	case 16:
		if (reg_index < 4) {
			name_sufix = 'X';
		} else if (reg_index >= 8) {
			name_sufix = 'W';
		}
		break;
	case 32:
		if (reg_index < 4) {
			name_prefix = 'E';
			name_sufix = 'X';
		} else if (reg_index < 8) {
			name_prefix = 'E';
		} else {
			name_sufix = 'D';
		}
		
		break;
	case 64:
		if (reg_index < 4) {
			name_prefix = 'R';
			name_sufix = 'X';
		} else if (reg_index < 8) {
			name_prefix = 'R';
		}
		break;
	}

	if (name_prefix) {
		str_builder_append_char(builder, name_prefix);
	}

	str_builder_append_cstr(builder, REG_BASE_NAMES[reg_index]);

	if (name_sufix) {
		str_builder_append_char(builder, name_sufix);
	}
}

static String _format_reg_names(Arena* allocator, uint16_t reg_mask, uint8_t reg_bit_count) {
	StringBuilder builder = { .arena = allocator };

	for (uint16_t i = 0; i < 16; i += 1) {
		if (!has_flag(reg_mask, 1 << i)) {
			continue;
		}
			
		if (builder.string.length > 0) {
			str_builder_append_char(&builder, ' ');
		}

		_format_reg_name(&builder, i, reg_bit_count);
	}

	return builder.string;
}

static InstrIndexArray _x64_gather_instr_with_storage_requirement(const InstrBuffer instr_buffer,
		const InstrUsageRange* usage_ranges,
		Arena* allocator) {

	InstrIndexArray result;
	result.count = 0;
	result.instr = arena_alloc_array(allocator, InstrIndex, 0);

	for (size_t i = 0; i < instr_buffer.count; i += 1) {
		const InstrKind kind = instr_buffer.instr[i].kind;
		if (!has_flag(INSTR_FEATURES[kind], INSTR_FEATURE_REG_STORAGE)) {
			continue;
		}

		if (usage_ranges[i].value == UINT32_MAX) {
			// Not used
			continue;
		}

		arena_alloc(allocator, InstrIndex);
		result.instr[result.count].value = (uint16_t)i;
		result.count += 1;
	}

	return result;
}

// Returned array stores a list of edges for each instruction in `instr_with_storage_requirement`
static UInt16Array* _x64_build_overlapping_instr_graph(const InstrBuffer instr_buffer,
		const InstrIndexArray instr_with_storage_requirement,
		const InstrUsageRange* usage_ranges,
		Arena* allocator) {

	UInt16Array* graph_edges = arena_alloc_array_zeroed(allocator,
			UInt16Array,
			instr_with_storage_requirement.count);

	for (size_t i = 0; i < instr_with_storage_requirement.count; i += 1) {
		UInt16Array* edges = &graph_edges[i];
		edges->values = arena_alloc_array(allocator, uint16_t, 0);
	
		InstrUsageRange usage_range_a = usage_ranges[instr_with_storage_requirement.instr[i].value];
		for (size_t j = 0; j < instr_with_storage_requirement.count; j += 1) {
			if (i == j) {
				continue;
			}

			InstrUsageRange usage_range_b = usage_ranges[instr_with_storage_requirement.instr[j].value];

			uint16_t max_start = max(usage_range_a.first_usage.value, usage_range_b.first_usage.value);
			uint16_t min_end = min(usage_range_a.last_usage.value, usage_range_b.last_usage.value);

			bool overlap = min_end > max_start;
			if (overlap) {
				arena_alloc(allocator, InstrIndex);
				edges->values[edges->count] = (uint16_t)j;
				edges->count += 1;
			}
		}
	}

	return graph_edges;
}

typedef struct {
	UInt16Array* clusters;
	size_t count;
} InstrOverlapClusters;

static InstrOverlapClusters _x64_build_overlapping_instr_clusters(const InstrIndexArray instr_with_storage_requirement,
		const UInt16Array* graph,
		Arena* allocator,
		Arena* temp_allocator) {

	size_t instr_count = instr_with_storage_requirement.count;
	
	// The sum of instruction counts in of all clusters is exactly
	// the size of `instr_with_storage_requirement`,
	// since the instruction can only belong to one cluster.
	//
	// Here we can just preallocate the buffer for all clusters,
	// although we don't know how many clusters we will end up with.

	UInt16Array cluster_instr_buffer;
	cluster_instr_buffer.values = arena_alloc_array(allocator, uint16_t, instr_count);
	cluster_instr_buffer.count = 0;

	UInt16Array* clusters = arena_alloc_array(allocator, UInt16Array, 0);
	size_t cluster_count = 0;

	ArenaRegion temp = arena_begin_temp(temp_allocator);

	BitArray visited_instr = bit_array_alloc(temp_allocator, instr_count);
	bit_array_clear(&visited_instr);

	UInt16Array visited_stack = uint16_array_alloc(temp_allocator, instr_count);
	size_t visited_stack_size = 0;

	for (size_t i = 0; i < instr_count; i += 1) {
		if (bit_array_get(&visited_instr, i)) {
			continue;
		}

		// Begin a new cluster
		size_t current_cluster_start = cluster_instr_buffer.count;

		assert(visited_stack_size < visited_stack.count);
		visited_stack.values[visited_stack_size] = (uint16_t)i;
		visited_stack_size += 1;

		while (visited_stack_size > 0) {
			visited_stack_size -= 1;
			uint16_t vertex_index = visited_stack.values[visited_stack_size];

			// Don't assert here, because the vertex might get pushed onto the stack
			// multiple times before it actually gets marked as visited, so when the loop
			// reaches the first occurance of the vertex on the stack, it processes it
			// and marks it as visited.
			//
			// However later when it reaches the next occurance, that vertex is already
			// visited and added to the cluster, so we should just skip it, instead of asserting.
			if (bit_array_get(&visited_instr, vertex_index)) {
				continue;
			}

			bit_array_set(&visited_instr, vertex_index, true);

			UInt16Array edges = graph[vertex_index];

			assert(cluster_instr_buffer.count < instr_count);
			cluster_instr_buffer.values[cluster_instr_buffer.count] = vertex_index;
			cluster_instr_buffer.count += 1;

			for (size_t j = 0; j < edges.count; j += 1) {
				if (bit_array_get(&visited_instr, edges.values[j])) {
					continue;
				}

				assert(visited_stack_size < visited_stack.count);
				visited_stack.values[visited_stack_size] = edges.values[j];
				visited_stack_size += 1;
			}
		}

		size_t current_cluster_size = cluster_instr_buffer.count - current_cluster_start;
		assert(current_cluster_size > 0);

		arena_alloc(allocator, UInt16Array);
		clusters[cluster_count].values = cluster_instr_buffer.values + current_cluster_start;
		clusters[cluster_count].count = current_cluster_size;
		cluster_count += 1;

		current_cluster_start = cluster_instr_buffer.count;
	}

	arena_end_temp(temp);

	InstrOverlapClusters result;
	result.clusters = clusters;
	result.count = cluster_count;
	return result;
}

void x64_alloc_registers(X64CodeGenerator* gen, uint16_t allowed_registers) {
	profile_scope_start(__func__);

	InstrIndexArray instr_with_storage_requirement = _x64_gather_instr_with_storage_requirement(
			gen->instr_buffer,
			gen->usage_ranges,
			gen->allocator);

	UInt16Array* graph = _x64_build_overlapping_instr_graph(gen->instr_buffer, 
			instr_with_storage_requirement,
			gen->usage_ranges,
			gen->allocator);

	for (size_t i = 0; i < instr_with_storage_requirement.count; i += 1) {
		UInt16Array overlap = graph[i];
		printf("%u:\n\t", (uint32_t)instr_with_storage_requirement.instr[i].value);

		for (size_t j = 0; j < overlap.count; j += 1) {
			InstrIndex overlapping_instr = instr_with_storage_requirement.instr[overlap.values[j]];
			printf("%u ", (uint32_t)overlapping_instr.value);
		}

		printf("\n");
	}

	InstrOverlapClusters clusters = _x64_build_overlapping_instr_clusters(instr_with_storage_requirement,
			graph,
			gen->allocator,
			gen->temp_allocator);

	for (size_t i = 0; i < clusters.count; i += 1) {
		printf("%zu: ", i);

		UInt16Array cluster = clusters.clusters[i];
		for (size_t j = 0; j < cluster.count; j += 1) {
			InstrIndex overlapping_instr = instr_with_storage_requirement.instr[cluster.values[j]];
			printf("%u ", (uint32_t)overlapping_instr.value);
		}

		printf("\n");
	}

	for (size_t i = 0; i < clusters.count; i += 1) {
		printf("%zu:\n", i);

		UInt16Array cluster = clusters.clusters[i];
		for (size_t j = 0; j < cluster.count; j += 1) {
			InstrIndex overlapping_instr = instr_with_storage_requirement.instr[cluster.values[j]];
			const Instr* instr = &gen->instr_buffer.instr[overlapping_instr.value];

			X64InstrStorageRequirement storage_requirement = s_instr_storage_requiremenets[instr->kind];

			ArenaRegion temp = arena_begin_temp(gen->allocator);

			uint16_t potential_instr_registers = storage_requirement.allowed_registers & allowed_registers;
			String allowed_registers_string = _format_reg_names(gen->allocator,
					potential_instr_registers,
					storage_requirement.reg_size);

			printf("\t%.*s\n", STR_FMT(allowed_registers_string));

			arena_end_temp(temp);
		}
	}

	InstrStorageLocation* instr_storage = arena_alloc_array_zeroed(gen->allocator,
			InstrStorageLocation,
			gen->instr_buffer.count);

	for (size_t i = 0; i < clusters.count; i += 1) {
		uint16_t allowed_cluster_registers = allowed_registers;
		UInt16Array cluster = clusters.clusters[i];

		for (size_t j = 0; j < cluster.count; j += 1) {
			InstrIndex overlapping_instr = instr_with_storage_requirement.instr[cluster.values[j]];
			const Instr* instr = &gen->instr_buffer.instr[overlapping_instr.value];

			X64InstrStorageRequirement storage_requirement = s_instr_storage_requiremenets[instr->kind];
			uint16_t potential_instr_registers = storage_requirement.allowed_registers & allowed_cluster_registers;

			assert(potential_instr_registers != 0);

			uint16_t first_potential_register = count_trailing_zeros(potential_instr_registers);
			assert(first_potential_register < 16);

			instr_storage[overlapping_instr.value].kind = INSTR_STORAGE_REG;
			instr_storage[overlapping_instr.value].reg = first_potential_register;

			allowed_cluster_registers &= ~(1 << first_potential_register);
		}
	}

	for (size_t i = 0; i < gen->instr_buffer.count; i += 1) {
		ArenaRegion temp = arena_begin_temp(gen->temp_allocator);

		String storage_string = STR_LIT("none");

		if (instr_storage[i].kind == INSTR_STORAGE_REG) {
			StringBuilder builder = { .arena = gen->temp_allocator };

			const InstrKind instr_kind = gen->instr_buffer.instr[i].kind;
			const X64InstrStorageRequirement storage_requirement = s_instr_storage_requiremenets[instr_kind];

			_format_reg_name(&builder, instr_storage[i].reg, storage_requirement.reg_size);
			storage_string = builder.string;
		}

		printf("\t%zu: %.*s\n", i, STR_FMT(storage_string));

		arena_end_temp(temp);
	}

	gen->instr_storage = instr_storage;

	profile_scope_end();
}

//
// CodeBuffer
//

static const size_t CODE_BUFFER_ALLOCATION_STEP = 64;
static const size_t CODE_BUFFER_ALLOCATION_STEP_MASK = CODE_BUFFER_ALLOCATION_STEP - 1;

typedef struct {
	uint8_t* buffer;
	size_t size;
	size_t capacity;
	Arena* allocator;
} CodeBuffer;

static void _code_buffer_init(CodeBuffer* buffer, Arena* allocator) {
	buffer->allocator = allocator;
	buffer->capacity = 0;
	buffer->size = 0;
	buffer->buffer = arena_alloc_array(allocator, uint8_t, 0);
}

static void _code_buffer_grow(CodeBuffer* buffer, size_t expected_capacity) {
	size_t capacity_delta = expected_capacity - buffer->capacity;
	size_t allocation_size = (capacity_delta + CODE_BUFFER_ALLOCATION_STEP - 1) & ~CODE_BUFFER_ALLOCATION_STEP_MASK;

	assert_msg(buffer->buffer + buffer->capacity == buffer->allocator->base + buffer->allocator->allocated,
			"Trying to grow CodeBuffer, however since the last grow there were allocations done, "
			"with the arena associated to this code buffer");

	arena_alloc_array(buffer->allocator, uint8_t, allocation_size);

	buffer->capacity += allocation_size;
}

inline uint8_t* _code_buffer_append(CodeBuffer* buffer, size_t byte_count) {
	if (buffer->size + byte_count > buffer->capacity) {
		_code_buffer_grow(buffer, buffer->size + byte_count);
	}

	uint8_t* bytes = buffer->buffer + buffer->size;
	buffer->size += byte_count;
	return bytes;
}

inline void _code_buffer_push_64(CodeBuffer* buffer, uint64_t value) {
	uint8_t* a = _code_buffer_append(buffer, sizeof(value));
	a[7] = (uint8_t)(value >> 56);
	a[6] = (uint8_t)((value >> 48) & 0xff);
	a[5] = (uint8_t)((value >> 40) & 0xff);
	a[4] = (uint8_t)((value >> 32) & 0xff);
	a[3] = (uint8_t)((value >> 24) & 0xff);
	a[2] = (uint8_t)((value >> 16) & 0xff);
	a[1] = (uint8_t)((value >> 8) & 0xff);
	a[0] = (uint8_t)((value >> 0) & 0xff);
}

inline void _code_buffer_push_32(CodeBuffer* buffer, uint32_t value) {
	uint8_t* a = _code_buffer_append(buffer, sizeof(value));
	a[3] = (uint8_t)((value >> 24) & 0xff);
	a[2] = (uint8_t)((value >> 16) & 0xff);
	a[1] = (uint8_t)((value >> 8) & 0xff);
	a[0] = (uint8_t)((value >> 0) & 0xff);
}

inline uint8_t _rex_prefix(uint8_t w, uint8_t r, uint8_t x, uint8_t b) {
	assert(w <= 1);
	assert(r <= 1);
	assert(x <= 1);
	assert(b <= 1);
	return 0b01000000 | (w << 3) | (r << 2) | (x << 1) | (b << 0);
}

inline uint8_t _rex_prefix_src_dst(uint8_t is_64_bit_reg, uint8_t src_reg, uint8_t dst_reg) {
	return _rex_prefix(is_64_bit_reg, src_reg >> 3, 0, dst_reg >> 3);
}

inline uint8_t _mod_rm(X64Register reg, uint8_t rm) {
	assert(reg < 8);
	assert(rm < 8);
	return 0b11000000 | (reg << 3) | (rm);
}

inline uint8_t _mod_rm_with_ext(uint8_t extension, uint8_t reg) {
	assert(extension < 8);
	assert(reg < 8);
	return 0b11000000 | (extension << 3) | (reg);
}

inline void _emit_load_const_64(CodeBuffer* buffer, X64Register reg, uint64_t value) {
	uint8_t* bytes = _code_buffer_append(buffer, 2);
	bytes[0] = 0b01000000 | (1 << 3) | (reg >> 3);
	bytes[1] = 0xb8 + (reg & 0b111);
	_code_buffer_push_64(buffer, value);
}

inline void _emit_return(CodeBuffer* buffer) {
	*_code_buffer_append(buffer, 1) = 0xc3;
}

inline void _emit_mov_regs(CodeBuffer* buffer, X64Register src, X64Register dst, uint8_t reg_bit_count) {
	if (src == dst) {
		return;
	}

	switch (reg_bit_count) {
	case 8:
	case 16:
	case 32:
		unreachable();
	case 64: {
		uint8_t rex_prefix = _rex_prefix_src_dst(1, src, dst);
		uint8_t rm = _mod_rm(src, dst);

		uint8_t* bytes = _code_buffer_append(buffer, 3);
		bytes[0] = rex_prefix;
		bytes[1] = 0x89;
		bytes[2] = rm;
		break;
	}
	default:
		 unreachable();
	}
}

inline void _emit_push_reg(CodeBuffer* buffer, X64Register reg, uint8_t reg_bit_count) {
	switch (reg_bit_count) {
	case 8:
	case 16:
	case 32:
		unreachable();
	case 64:
		// The register index dones't fit in 3-bit,
		// so we need a REX prefix with R set 1,
		// for an extension of the register index in MODRM
		uint8_t* bytes = _code_buffer_append(buffer, 2);
		if (reg >= 8) {
			bytes[0] = _rex_prefix(0, 0, (reg >> 3), 1);
			bytes[1] = 0x50 + (reg & 0b111);
		} else {
			bytes[0] = 0xff;
			bytes[1] = _mod_rm_with_ext(6, reg);
		}

		break;
	default:
		unreachable();
	}
}

inline void _emit_pop_reg(CodeBuffer* buffer, X64Register dst_reg, uint8_t reg_bit_count) {

	switch (reg_bit_count) {
	case 8:
	case 16:
	case 32:
		unreachable();
	case 64:
		// The register index dones't fit in 3-bit,
		// so we need a REX prefix with R set 1,
		// for an extension of the register index in MODRM

		uint8_t* bytes = _code_buffer_append(buffer, 2);
		if (dst_reg >= 8) {
			bytes[0] = _rex_prefix(0, 0, (dst_reg >> 3), 1);
			bytes[1] = 0x58 + (dst_reg & 0b111);
		} else {
			bytes[0] = 0x8f;
			bytes[1] = _mod_rm_with_ext(0, dst_reg);
		}
		break;
	default:
		unreachable();
	}
}

static void _emit_sub_rsp(CodeBuffer* buffer, uint32_t offset) {
	if (offset == 0) {
		return;
	}

	uint8_t* bytes = _code_buffer_append(buffer, 3);
	bytes[0] = _rex_prefix(1, 0, 0, 0);
	bytes[1] = 0x81;
	bytes[2] = _mod_rm_with_ext(5, REG_SP);

	_code_buffer_push_32(buffer, offset);
}

static void _emit_add_rsp(CodeBuffer* buffer, uint32_t offset) {
	if (offset == 0) {
		return;
	}

	uint8_t* bytes = _code_buffer_append(buffer, 3);
	bytes[0] = _rex_prefix(1, 0, 0, 0);
	bytes[1] = 0x81;
	bytes[2] = _mod_rm_with_ext(0, REG_SP);

	_code_buffer_push_32(buffer, offset);
}

int _internal_assert(uint64_t predicate) {
	assert(predicate);
	return 0;
}

//
// Code Generation
//

void _x64_generate_code(X64CodeGenerator* gen, InstrIndex instr_index, CodeBuffer* buffer) {
	assert(instr_index.value < gen->instr_buffer.count);

	const Instr* instr = &gen->instr_buffer.instr[instr_index.value];
	const InstrStorageLocation instr_storage = gen->instr_storage[instr_index.value];

	switch (instr->kind) {
	case INSTR_NO_OP:
		break;

	case INSTR_CONST_8:
	case INSTR_CONST_16:
	case INSTR_CONST_32:
		unreachable();

	case INSTR_CONST_64:
		assert(instr_storage.kind == INSTR_STORAGE_REG);
		_emit_load_const_64(buffer, instr_storage.reg, instr->const_64.u);
		break;

	case INSTR_BIN_OP_8:
	case INSTR_BIN_OP_16:
	case INSTR_BIN_OP_32:
		unreachable();
	case INSTR_BIN_OP_64: {
		_x64_generate_code(gen, instr->bin_op.left, buffer);
		_x64_generate_code(gen, instr->bin_op.right, buffer);

		const InstrStorageLocation dst_loc = gen->instr_storage[instr_index.value];
		const InstrStorageLocation left_loc = gen->instr_storage[instr->bin_op.left.value];
		const InstrStorageLocation right_loc = gen->instr_storage[instr->bin_op.right.value];
		assert(dst_loc.kind == INSTR_STORAGE_REG);
		assert(left_loc.kind == INSTR_STORAGE_REG);
		assert(right_loc.kind == INSTR_STORAGE_REG);

		_emit_mov_regs(buffer, left_loc.reg, dst_loc.reg, 64);

		uint8_t rex_prefix = _rex_prefix_src_dst(1, dst_loc.reg, right_loc.reg);
		uint8_t mod_rm = _mod_rm(dst_loc.reg, right_loc.reg);

		uint8_t* instr_bytes = _code_buffer_append(buffer, 3);
		instr_bytes[0] = rex_prefix;
		instr_bytes[2] = mod_rm;

		switch (instr->bin_op.kind) {
		case INSTR_BIN_ADD:
			instr_bytes[1] = 0x03;
			break;
		}
		break;
	}

	case INSTR_BRANCH:
	case INSTR_JUMP:
		break;

	case INSTR_RETURN_VALUE:
		_x64_generate_code(gen, instr->ret.value, buffer);
		const InstrStorageLocation return_value_loc = gen->instr_storage[instr->ret.value.value];
		assert(return_value_loc.kind == INSTR_STORAGE_REG);

		_emit_mov_regs(buffer, return_value_loc.reg, REG_A, 64);

		_emit_return(buffer);
		break;
	
	case INSTR_CALL_INTERNAL: {
		assert(instr_storage.kind == INSTR_STORAGE_REG);

		const uint32_t SHADOW_SPACE_SIZE = 32;

		X64Register saved_registers[] = {
			REG_A,
			REG_C,
			REG_D,
			REG_8,
			REG_9,
			REG_10,
			REG_11,
		};

		_x64_generate_code(gen, instr->call_internal.arg, buffer);

		// Push saved registers
		for (size_t i = 0; i < array_size(saved_registers); i += 1) {
			_emit_push_reg(buffer, saved_registers[i], 64);
		}

		InstrIndex arg_instr_index = instr->call_internal.arg;
		const InstrStorageLocation arg_storage_loc = gen->instr_storage[arg_instr_index.value];
		_emit_mov_regs(buffer, arg_storage_loc.reg, REG_C, 64);

		_emit_load_const_64(buffer, REG_A, (uint64_t)_internal_assert);

		// push shadow space
		_emit_sub_rsp(buffer, SHADOW_SPACE_SIZE);

		// call
		uint8_t* instr_bytes = _code_buffer_append(buffer, 2);
		instr_bytes[0] = 0xff;
		instr_bytes[1] = _mod_rm_with_ext(2, 0);

		// pop shadow space
		_emit_add_rsp(buffer, SHADOW_SPACE_SIZE);

		// Now move the return value into a the proper register dedicated
		// exactly for the return value of this call instruction
		_emit_mov_regs(buffer, REG_A, instr_storage.reg, 64);

		// Pop saved registers in reverse order
		for (size_t i = array_size(saved_registers); i > 0; i -= 1) {
			X64Register reg = saved_registers[i - 1];
			bool should_restore = instr_storage.reg != reg;
			if (should_restore) {
				_emit_pop_reg(buffer, reg, 64);
			} else {
				_emit_add_rsp(buffer, 8);
			}
		}

		break;
	}

	case INSTR_REGION:
		_x64_generate_code(gen, instr->region.last_instr, buffer);
		break;
	}
}

void x64_generate_code(X64CodeGenerator* gen, InstrIndex root_region) {
	CodeBuffer buffer;
	_code_buffer_init(&buffer, gen->allocator);

	_x64_generate_code(gen, root_region, &buffer);

	void* executable_memory = allocate_executable(buffer.size);
	memcpy(executable_memory, buffer.buffer, buffer.size);

	typedef uint64_t (*ExecutableFunction)();
	ExecutableFunction function = (ExecutableFunction)executable_memory;

	uint64_t result = function();

	free_executable(executable_memory, buffer.size);

	printf("%llu", result);
}
