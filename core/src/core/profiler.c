#include "profiler.h"

static ProfileState s_profile_state;

ProfileState* profile_get_state() {
	return &s_profile_state;
}

void profile_init(size_t capacity) {
	s_profile_state.count = 0;
	s_profile_state.capacity = capacity;
	s_profile_state.scopes = heap_alloc_array(ProfileScopeTimer, capacity);
}

void profile_finish() {
	uint64_t timer_freq = hardware_timer_get_frequency();
	uint64_t ticks_per_sec = timer_freq * 1000;
	uint64_t ticks_per_ms = timer_freq;
	uint64_t ticks_per_micro_sec = timer_freq / 1000;
	uint64_t ticks_per_ns = timer_freq / 1000000;

	for (size_t i = 0; i < s_profile_state.count; i += 1) {
		const ProfileScopeTimer* scope = &s_profile_state.scopes[i];

		uint64_t duration_in_ticks = (scope->end_time - scope->start_time);
		uint64_t duration = 0;
		const char* duration_sufix = "ns";

		if (duration_in_ticks >= ticks_per_sec) {
			duration = duration_in_ticks / ticks_per_sec;
			duration_sufix = "s";
		} else if (duration_in_ticks >= ticks_per_ms) {
			duration = duration_in_ticks / ticks_per_ms;
			duration_sufix = "ms";
		} else if (duration_in_ticks >= ticks_per_micro_sec) {
			duration = duration_in_ticks / ticks_per_micro_sec;
			duration_sufix = "micro sec";
		} else {
			duration = duration_in_ticks / ticks_per_ns;
			duration_sufix = "ns";
		}
		
		printf("%s \033[33;1m%s:%u\033[0m - %llu%s\n", scope->name, scope->file, scope->line, duration, duration_sufix);
	}

	heap_release(s_profile_state.scopes);
}
