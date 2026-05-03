#include "x64.h"

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

static InstrIndexArray _x64_gather_overlapping_instr_into_cluster(const InstrBuffer instr_buffer,
		const InstrUsageRange* usage_ranges,
		Arena* allocator) {

}

typedef struct {
	InstrIndexArray* edges;
} InstrOverlapGraph;

// Returned array stores a list of edges for each instruction in `instr_with_storage_requirement`
static InstrIndexArray* _x64_build_overlapping_instr_graph(const InstrBuffer instr_buffer,
		InstrIndexArray instr_with_storage_requirement,
		const InstrUsageRange* usage_ranges,
		Arena* allocator) {

	InstrIndexArray* graph_edges = arena_alloc_array_zeroed(allocator,
			InstrIndexArray,
			instr_with_storage_requirement.count);

	for (size_t i = 0; i < instr_with_storage_requirement.count; i += 1) {
		InstrIndexArray* edges = &graph_edges[i];
		edges->instr = arena_alloc_array(allocator, InstrIndex, 0);
	
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
				edges->instr[edges->count] = instr_with_storage_requirement.instr[j];
				edges->count += 1;
			}
		}
	}

	return graph_edges;
}

void x64_alloc_registers(X64CodeGenerator* gen) {
	InstrIndexArray instr_with_storage_requirement = _x64_gather_instr_with_storage_requirement(
			gen->instr_buffer,
			gen->usage_ranges,
			gen->allocator);

	InstrIndexArray* graph = _x64_build_overlapping_instr_graph(gen->instr_buffer, 
			instr_with_storage_requirement,
			gen->usage_ranges,
			gen->allocator);

	for (size_t i = 0; i < instr_with_storage_requirement.count; i += 1) {
		InstrIndexArray overlap = graph[i];
		printf("%u:\n\t", (uint32_t)instr_with_storage_requirement.instr[i].value);

		for (size_t j = 0; j < overlap.count; j += 1) {
			InstrIndex overlapping_instr = overlap.instr[j];
			printf("%u ", (uint32_t)overlapping_instr.value);
		}

		printf("\n");
	}
}
