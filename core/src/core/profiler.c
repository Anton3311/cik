#include "profiler.h"
#include "threads.h"

#ifdef FEATURE_PROFILER

void profiler_wait_for_connection() {
	while (!TracyCIsConnected) {
		thrd_yield();
	}
}

#endif
