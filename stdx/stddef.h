#pragma once

#define NULL (void*)0
#define offsetof(type, field) (size_t)(&((type*)NULL)->field)

typedef short wchar_t;
