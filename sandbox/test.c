#if defined hello
hello
#elif !10
world
#endif

#define fmt(...) __VA_ARGS__
fmt(10, 11, 12);
