#define inner(a, b) a + b
#define inner2(a, b) inner(a + b) + b
#define outer(a, b, c) (inner2(a, b) + inner2(b, a)) * c
outer(1, 2, 3)
