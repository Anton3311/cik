#include "x64_linker.h"

#include "core/core.h"

LinkedProgram linker_link(const LoweredFunction* functions,
		size_t function_count,
		const FunctionRefTable* ref_table,
		Arena* allocator) {
	profile_scope_start(__func__);

	MachineCodeBuffer machine_code = {};

	size_t* function_offsets = arena_alloc_array(allocator, size_t, function_count);

	{
		size_t offset = 0;
		for (size_t i = 0; i < function_count; i += 1) {
			function_offsets[i] = offset;

			machine_code.size_in_bytes += functions[i].size_in_bytes;
			offset += functions[i].size_in_bytes;
		}
	}

	machine_code.code = allocate_executable(machine_code.size_in_bytes);

	for (size_t i = 0; i < function_count; i += 1) {
		const LoweredFunction* func = &functions[i];

		void* function_offset = (uint8_t*)machine_code.code + function_offsets[i];
		memcpy(function_offset, func->code, func->size_in_bytes);

		size_t placeholder_count = func->call_addr_placeholder_count;
		for (size_t placeholder_index = 0;
				placeholder_index < placeholder_count;
				placeholder_index += 1) {

			CallAddressPlaceholder placeholder = func->call_addr_placeholders[placeholder_index];
			
			uint64_t instruction_end_offset = (uint64_t)placeholder.instruction_end_offset + (uint64_t)function_offset;

			const FunctionRef* callee = &ref_table->refs[placeholder.function_index];
			uint64_t callee_address;

			switch (callee->impl_kind) {
			case FUNCTION_IMPL_INTERNAL:
				callee_address = (uint64_t)machine_code.code + function_offsets[callee->internal_function_index];
				break;
			case FUNCTION_IMPL_EXTERNAL:
				callee_address = (uint64_t)callee->external_address;
				break;
			case FUNCTION_IMPL_NONE:
				printf("unresolved reference to '%.*s'\n", STR_FMT(callee->name));
				unreachable();
			}

			void* addr_offset = (uint8_t*)function_offset + placeholder.addr_offset;

			switch (placeholder.kind) {
			case CALL_ADDR_ABSOLUTE: {
				memcpy(addr_offset, &callee_address, sizeof(callee_address));
				break;
			}
			case CALL_ADDR_RELATIVE: {
				assert(callee_address <= INT64_MAX);
				assert(instruction_end_offset <= INT64_MAX);

				int64_t relative_offset = (int64_t)callee_address - (int64_t)instruction_end_offset;
				assert(INT32_MIN <= relative_offset && relative_offset <= INT32_MAX);

				int32_t relative_offset_32 = (int32_t)relative_offset;

				memcpy(addr_offset, &relative_offset_32, sizeof(relative_offset_32));
				break;
			}
			}

		}
	}

	LinkedProgram linked = {};
	linked.machine_code = machine_code;
	linked.function_offsets = function_offsets;

	profile_scope_end();
	return linked;
}
