#include "x64_reg_alloc.h"

typedef struct Bundle Bundle;
typedef struct BundleInstrChunk BundleInstrChunk;

#define BUNDLE_INSTR_CHUNK_CAPACITY 24

struct BundleInstrChunk {
	InstrIndex buffer[BUNDLE_INSTR_CHUNK_CAPACITY];
	size_t count;
	BundleInstrChunk* next;
};

struct Bundle {
	size_t instr_count;
	BundleInstrChunk* chunk;
	Bundle* next;
};

static void _bundle_append(Bundle* bundle, InstrIndex instr_index, Arena* allocator) {
	if (bundle->chunk == NULL || bundle->chunk->count == BUNDLE_INSTR_CHUNK_CAPACITY) {
		BundleInstrChunk* chunk = arena_alloc(allocator, BundleInstrChunk);
		chunk->buffer[0] = instr_index;
		chunk->count = 1;
		chunk->next = bundle->chunk;

		bundle->chunk = chunk;
	} else {
		assert(bundle->chunk);

		BundleInstrChunk* chunk = bundle->chunk;
		assert(chunk->count <= BUNDLE_INSTR_CHUNK_CAPACITY);

		chunk->buffer[chunk->count] = instr_index;
		chunk->count += 1;
	}

	bundle->instr_count += 1;
}

static bool _phi_has_value_as_variant(const InstrBuffer* instr_buffer,
		const Instr* phi_instr,
		InstrIndex value_instr) {
	profile_scope_start(__func__);
	assert(phi_instr->kind == INSTR_PHI);

	InstrInputs variants = phi_instr->phi.variants;
	for (uint16_t i = 0; i < variants.count; i += 1) {
		InstrIndex select_index = instr_buffer->inputs_buffer[variants.start + i];
		const Instr* select = instr_buffer_at(instr_buffer, select_index);

		if (select->select.value.value == value_instr.value) {
			profile_scope_end();
			return true;
		}
	}

	profile_scope_end();
	return false;
}

static bool _bundle_can_accept_instr(const InstrBuffer* instr_buffer,
		const Bundle* bundle,
		InstrIndex instr_index,
		const InstrLiveRange* live_ranges) {
	profile_scope_start(__func__);

	for (const BundleInstrChunk* chunk = bundle->chunk; chunk != NULL; chunk = chunk->next) {
		for (size_t i = 0; i < chunk->count; i += 1) {
			InstrLiveRange live_range_a = live_ranges[instr_index.value];
			InstrLiveRange live_range_b = live_ranges[chunk->buffer[i].value];
			assert(live_range_a.value != UINT32_MAX);
			assert(live_range_b.value != UINT32_MAX);

			uint16_t max_start = max(live_range_a.start, live_range_b.start);
			uint16_t min_end = min(live_range_a.end, live_range_b.end);

			// Check whether live ranges overlap.
			bool overlap = min_end >= max_start;

			// If the ranges only overlap at their ends, then don't consider them overlapping
			if (live_range_a.end == live_range_b.start) {
				overlap = false;
			}

			if (live_range_a.start == live_range_b.end) {
				overlap = false;
			}

			if (!overlap) {
				continue;
			}

			InstrIndex instr_index_b = chunk->buffer[i];

			const Instr* instr_a = instr_buffer_at(instr_buffer, instr_index);
			const Instr* instr_b = instr_buffer_at(instr_buffer, instr_index_b);

			// Make sure that phis and their variants don't overlap.
			if (instr_a->kind == INSTR_PHI
					&& _phi_has_value_as_variant(instr_buffer, instr_a, instr_index_b)) {
				debug_log_info("can overlap phi '%u' with its variant '%u'",
						instr_index.value,
						instr_index_b.value);
				continue;
			}

			if (instr_b->kind == INSTR_PHI
					&& _phi_has_value_as_variant(instr_buffer, instr_b, instr_index)) {
				debug_log_info("can overlap phi '%u' with its variant '%u'",
						instr_index_b.value,
						instr_index.value);
				continue;
			}

			profile_scope_end();
			return false;
		}
	}

	profile_scope_end();
	return true;
}

typedef struct {
	Bundle* bundles;

	const InstrLiveRange* live_ranges;
	const InstrBuffer* instr_buffer;
	Bundle** assigned_bundles;
} BundleBuildContext;

static Bundle* _build_bundles(const InstrBuffer* instr_buffer,
		const InstrLiveRange* live_ranges,
		const InstrIndexArray scheduled_instr,
		Arena* allocator) {
	profile_scope_start(__func__);

	BundleBuildContext context = {};
	context.bundles = arena_alloc_zeroed(allocator, Bundle);
	context.live_ranges = live_ranges;
	context.instr_buffer = instr_buffer;
	context.assigned_bundles = arena_alloc_array_zeroed(allocator, Bundle*, instr_buffer->count);

	for (size_t i = 0; i < scheduled_instr.count; i += 1) {
		InstrIndex instr_index = scheduled_instr.instr[i];
		const Instr* instr = instr_buffer_at(instr_buffer, instr_index);

		InstrFeatureFlag feature_flags = INSTR_FEATURES[instr->kind];

		if (!has_flag(feature_flags, INSTR_FEATURE_REG_STORAGE)
				&& !has_flag(feature_flags, INSTR_FEATURE_STACK_STORAGE)) {
			continue;
		}

		Bundle* selected_bundle = NULL;
		for (Bundle* bundle = context.bundles; bundle != NULL; bundle = bundle->next) {
			bool can_accept = _bundle_can_accept_instr(instr_buffer,
					bundle,
					instr_index,
					live_ranges);

			if (can_accept && selected_bundle) {
				if (bundle->instr_count > selected_bundle->instr_count) {
					selected_bundle = bundle;
				}
			} else if (can_accept) {
				selected_bundle = bundle;
			}
		}

		if (selected_bundle == NULL) {
			Bundle* bundle = arena_alloc_zeroed(allocator, Bundle);
			bundle->next = context.bundles;
			context.bundles = bundle;

			selected_bundle = bundle;
		}

		_bundle_append(selected_bundle, instr_index, allocator);
		context.assigned_bundles[instr_index.value] = selected_bundle;
	}

	profile_scope_end();
	return context.bundles;
}

// Returned array stores an array of edges for each instruction in `instr_with_storage_requirement`
//
// The array must be indexed using an element index of the `instr_with_storage_requirement`
static InstrIndexArray* _build_interference_graph(const InstrBuffer* instr_buffer,
		const InstrLiveRange* live_ranges,
		Arena* allocator) {

	profile_scope_start(__func__);

	// Each array stores indices into `instr_with_storage_requirement`
	InstrIndexArray* graph_edges = arena_alloc_array_zeroed(allocator,
			InstrIndexArray,
			instr_buffer->count);

	for (uint16_t i = 0; i < instr_buffer->count; i += 1) {
		InstrIndexArray* edges = &graph_edges[i];
		edges->instr = arena_alloc_array(allocator, InstrIndex, 0);
	
		InstrLiveRange live_range_a = live_ranges[i];
		if (live_range_a.value == UINT32_MAX) {
			continue;
		}

		for (uint16_t j = 0; j < instr_buffer->count; j += 1) {
			if (i == j) {
				continue;
			}

			InstrLiveRange live_range_b = live_ranges[j];
			if (live_range_b.value == UINT32_MAX) {
				continue;
			}

			uint16_t max_start = max(live_range_a.start, live_range_b.start);
			uint16_t min_end = min(live_range_a.end, live_range_b.end);

			// Check whether live ranges overlap.
			bool overlap = min_end >= max_start;

#if 1
			// If the ranges only overlap at their ends, then don't consider them overlapping
			if (live_range_a.end == live_range_b.start) {
				overlap = false;
			}

			if (live_range_a.start == live_range_b.end) {
				overlap = false;
			}
#endif

			if (overlap) {
				arena_alloc(allocator, InstrIndex);
				edges->instr[edges->count] = (InstrIndex) { j };
				edges->count += 1;
			}
		}
	}

	profile_scope_end();
	return graph_edges;
}

inline bool _instr_allowed_to_share_a_register(InstrLiveRange live_range_a,
		InstrLiveRange live_range_b) {
	if (live_range_a.end == live_range_b.start) {
		return true;
	}

	if (live_range_a.start == live_range_b.end) {
		return true;
	}

	return false;
}

// Writes storage locations into `instr_storage` array.
//
// This array is expected to be of size `instr_buffer.count`
static bool _run_graph_coloring(const InstrBuffer* instr_buffer,
		const InstrIndexArray scheduled_instr,
		const InstrLiveRange* live_ranges,
		const InstrIndexArray* interference_graph,
		uint16_t allowed_registers,
		const AbiSignature* current_function_signature,
		const AbiSignature* function_signatures,
		Arena* allocator,
		Arena* temp_allocator,
		RegisterAllocationResult* out_result) {
	profile_scope_start(__func__);
	ArenaRegion temp = arena_begin_temp(temp_allocator);

	InstrStorageLocation* instr_storage = arena_alloc_array_zeroed(allocator,
			InstrStorageLocation,
			instr_buffer->count);

	uint16_t* potential_instr_registers = arena_alloc_array(temp_allocator,
			uint16_t,
			instr_buffer->count);

	bool graph_coloring_result = true;
	for (size_t i = 0; i < scheduled_instr.count; i += 1) {
		InstrIndex instr_index = scheduled_instr.instr[i];

		// Skip dead instructions
		InstrKind kind = instr_buffer->instr[instr_index.value].kind;
		if (!has_flag(INSTR_FEATURES[kind], INSTR_FEATURE_REG_STORAGE)) {
			continue;
		}

		if (kind == INSTR_LOAD_ARG_8
				|| kind == INSTR_LOAD_ARG_16
				|| kind == INSTR_LOAD_ARG_32
				|| kind == INSTR_LOAD_ARG_64
				|| kind == INSTR_LOAD_ARG_STACK) {
			continue;
		}

		if (has_flag(INSTR_FEATURES[kind], INSTR_FEATURE_REG_STORAGE)) {
			potential_instr_registers[instr_index.value] = allowed_registers;

			assert_msg(potential_instr_registers[instr_index.value] != 0,
					"This instruction must be spilled, but spilling is not yet implemented");
		}
	}

	CallFrameLayout frame_layout = compute_call_frame_layout(
			current_function_signature,
			temp_allocator);

	size_t argument_location_count = 0;
	InstrStorageLocation* argument_locations = arena_alloc_array(temp_allocator,
			InstrStorageLocation, 0);

	for (uint32_t i = 0; i < current_function_signature->param_count; i += 1) {
		if (current_function_signature->params[i].kind == ABI_PARAM_RETURN_LOCATION) {
			continue;
		}

		arena_alloc(temp_allocator, InstrStorageLocation);
		argument_locations[argument_location_count] = frame_layout.locations[i];
		argument_location_count += 1;
	}

	// Assign locations to function arguments.
	// These locations are determined by the calling convention.
	for (size_t i = 0; i < scheduled_instr.count; i += 1) {
		InstrIndex instr_index = scheduled_instr.instr[i];

		InstrKind kind = instr_buffer->instr[instr_index.value].kind;
		if (kind != INSTR_LOAD_ARG_8
				&& kind != INSTR_LOAD_ARG_16
				&& kind != INSTR_LOAD_ARG_32
				&& kind != INSTR_LOAD_ARG_64
				&& kind != INSTR_LOAD_ARG_STACK) {
			continue;
		}

		const Instr* instr = instr_buffer_at(instr_buffer, instr_index);

		assert(instr->load_arg.index < argument_location_count);
		instr_storage[instr_index.value] = argument_locations[instr->load_arg.index];

		if (instr_storage[instr_index.value].kind != INSTR_STORAGE_REG) {
			continue;
		}

		// If the argument is placed in the register, we need to go through the interfering
		// instructions, and disallow the selected register for them.
		X64Register reg = instr_storage[instr_index.value].reg;

		InstrIndexArray edges = interference_graph[instr_index.value];
		for (size_t j = 0; j < edges.count; j += 1) {
			InstrIndex interfering_instr = edges.instr[j];
			potential_instr_registers[interfering_instr.value] &= ~(1 << reg);
		}
	}

	// Assign locations to the rest of the instructions
	uint32_t stack_offset = 0;
	for (size_t i = 0; i < scheduled_instr.count; i += 1) {
		InstrIndex instr_index = scheduled_instr.instr[i];

		if (instr_storage[instr_index.value].kind != INSTR_STORAGE_NONE) {
			continue;
		}

		const Instr* instr = instr_buffer_at(instr_buffer, instr_index);

		if (instr->kind == INSTR_CALL_DIRECT || instr->kind == INSTR_CALL_INDIRECT) {
			AbiSignature signature;

			if (instr->kind == INSTR_CALL_DIRECT) {
				signature = function_signatures[instr->call_direct.signature_index];
			} else if (instr->kind == INSTR_CALL_INDIRECT) {
				signature = function_signatures[instr->call_indirect.signature_index];
			}

			if (signature.returns != NULL) {
				if (signature.returns->kind == ABI_PARAM_STRUCT
						&& signature.returns->struct_size <= 8) {
					// Go through the usual allocator path
				} else if (signature.returns->kind == ABI_PARAM_STRUCT) {
					assert(signature.returns->struct_size > 8);
					stack_offset = align(stack_offset, 16); // FIXME: No hardcoded alignment

					instr_storage[instr_index.value].kind = INSTR_STORAGE_STACK;
					instr_storage[instr_index.value].stack.offset = stack_offset;
					stack_offset += signature.returns->struct_size;
					continue;
				} else if (signature.returns->kind == ABI_PARAM_NORMAL) {
					// Go through the usual allocator path
				} else {
					panic("Invalid 'AbiParam' for the functions return");
				}
			}
		}

		if (has_flag(INSTR_FEATURES[instr->kind], INSTR_FEATURE_STACK_STORAGE)) {
			assert(instr->stack_alloc.alignment > 0);
			assert(is_power_of_2(instr->stack_alloc.alignment));

			stack_offset = align(stack_offset, instr->stack_alloc.alignment);

			instr_storage[instr_index.value].kind = INSTR_STORAGE_STACK;
			instr_storage[instr_index.value].stack.offset = stack_offset;
			stack_offset += instr->stack_alloc.size;
		} else if (has_flag(INSTR_FEATURES[instr->kind], INSTR_FEATURE_REG_STORAGE)) {
			uint16_t potential_registers = potential_instr_registers[instr_index.value];

			if (potential_registers == 0) {
				graph_coloring_result = false;
				debug_log_error("'%u' must be spilled. Live range: [%u; %u]",
						instr_index.value,
						live_ranges[instr_index.value].start,
						live_ranges[instr_index.value].end);
				continue;
			}

			uint16_t first_potential_register = count_trailing_zeros(potential_registers);
			assert(first_potential_register < 16);

			instr_storage[instr_index.value].kind = INSTR_STORAGE_REG;
			instr_storage[instr_index.value].reg = first_potential_register;

			InstrIndexArray edges = interference_graph[instr_index.value];
			for (size_t j = 0; j < edges.count; j += 1) {
				// TODO: Maybe skip modifing `potential_instr_registers` for instructions that don't
				//       have storage?
				potential_instr_registers[edges.instr[j].value] &= ~(1 << first_potential_register);
			}
		} else {
			// This instruction has no storage
			continue;
		}
	}

	out_result->allocations = instr_storage;
	out_result->stack_usage = stack_offset;

	arena_end_temp(temp);
	profile_scope_end();

	return graph_coloring_result;
}

RegisterAllocationResult x64_alloc_regs(const InstrBuffer* instr_buffer,
		const InstrIndexArray scheduled_instr,
		InstrLiveRange* live_ranges,
		uint16_t allowed_registers,
		const AbiSignature* current_function_signature,
		const AbiSignature* function_signatures,
		Arena* allocator,
		Arena* temp_allocator) {
	profile_scope_start(__func__);

	// Disallow any registers that are used for the return area address
	//
	// For __cdecl calling convention, in case the function wants to return a struct bigger than a
	// register size, the first argument register is used to pass the address of the area, where the
	// returned struct should be written to.
	ArenaRegion temp = arena_begin_temp(temp_allocator);
	CallFrameLayout frame_layout = compute_call_frame_layout(
			current_function_signature,
			temp_allocator);

	for (uint32_t i = 0; i < current_function_signature->param_count; i += 1) {
		AbiParam param = current_function_signature->params[i];
		InstrStorageLocation location = frame_layout.locations[i];
		if (location.kind == INSTR_STORAGE_REG && param.kind == ABI_PARAM_RETURN_LOCATION) {
			allowed_registers &= ~(1 << location.reg);
		}
	}

	Bundle* bundles = _build_bundles(instr_buffer, live_ranges, scheduled_instr, temp_allocator);
	size_t bundle_index = 0;
	for (Bundle* bundle = bundles; bundle != NULL; bundle = bundle->next, bundle_index += 1) {
		printf("bundle %zu:\n", bundle_index);
		for (BundleInstrChunk* chunk = bundle->chunk; chunk != NULL; chunk = chunk->next) {
			for (size_t i = 0; i < chunk->count; i += 1) {
				printf("  instr '%%%u' [%u; %u]:\n",
						chunk->buffer[i].value,
						live_ranges[chunk->buffer[i].value].start,
						live_ranges[chunk->buffer[i].value].end);
			}
		}
	}

	arena_end_temp(temp);

	InstrIndexArray* interference_graph = _build_interference_graph(instr_buffer,
			live_ranges,
			temp_allocator);

	RegisterAllocationResult result;
	result.interference_graph = interference_graph;

	bool graph_coloring_result = _run_graph_coloring(instr_buffer,
			scheduled_instr,
			live_ranges,
			interference_graph,
			allowed_registers,
			current_function_signature,
			function_signatures,
			allocator,
			temp_allocator,
			&result);

	assert(graph_coloring_result);

	profile_scope_end();
	return result;
}
