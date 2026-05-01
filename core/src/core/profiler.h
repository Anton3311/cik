#ifndef PROFILER_H
#define PROFILER_H

#include <intrin.h>

#include "core/core.h"

typedef struct {
	uint64_t start_time;
	uint64_t end_time;
	const char* name;
	const char* file;
	uint32_t line;
} ProfileScopeTimer;

typedef struct {
	ProfileScopeTimer dummy_timer;

	ProfileScopeTimer* scopes;
	size_t count;
	size_t capacity;
} ProfileState;

ProfileState* profile_get_state();

void profile_init(size_t capacity);
void profile_finish();

inline ProfileScopeTimer* profiler_append(uint64_t start_time, const char* name, const char* file, uint32_t line) {
	ProfileState* state = profile_get_state();
	if (state->scopes == NULL) {
		return &state->dummy_timer;
	}

	assert(state->count < state->capacity);

	ProfileScopeTimer* timer = &state->scopes[state->count];
	state->count += 1;
	
	timer->start_time = start_time;
	timer->name = name;
	timer->file = file;
	timer->line = line;
	return timer;
}

#define profile_scope_start(scope_name) \
	ProfileScopeTimer* __scope_timer__ = profiler_append(__rdtsc(), scope_name, __FILE__, __LINE__)
#define profile_scope_end() __scope_timer__->end_time = __rdtsc();

#endif
