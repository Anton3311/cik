#define assert(expression) if (!(expression)) { printf("%s:%u", __FILE__, __LINE__); }
assert(true)
