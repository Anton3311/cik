#define ignore(a, b, c, d)
#define macro(a, b, c, d) ignore(a, b##_s, c, d)

macro(first hello world, second, third hello world, fours)

pass
