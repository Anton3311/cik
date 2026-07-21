#ifndef PROFILER_H
#define PROFILER_H

#ifdef FEATURE_PROFILER
	#include "tracy/TracyC.h"

	#define profile_scope_start(scope_name) TracyCZoneN(___tracy_scoped_zone, scope_name, true)
	#define profile_func_colored(color) TracyCZoneC(___tracy_scoped_zone, color, true)
	#define profile_scope_end() TracyCZoneEnd(___tracy_scoped_zone)

	void profiler_wait_for_connection();
#else
	#define profile_scope_start(scope_name)
	#define profile_func_colored(color)
	#define profile_scope_end()
#endif

#endif
