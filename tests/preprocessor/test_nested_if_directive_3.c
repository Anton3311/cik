#ifdef OUTER

	#ifndef SHOULD_PASS
		#define SHOULD_PASS 0
	#endif

#else

	#ifndef SHOULD_PASS
		#define SHOULD_PASS 1
	#endif

#endif

#if SHOULD_PASS
pass
#else
fail
#endif
