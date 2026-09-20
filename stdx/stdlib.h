#pragma once

#define NULL (void*)0

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

__declspec(dllimport) void exit(int);
__declspec(dllimport) void* malloc(size_t);
__declspec(dllimport) void free(void*);

__declspec(dllimport) void memcpy(void* dst, const void* src, size_t size);
__declspec(dllimport) void memset(void* dst, int value, size_t size);
__declspec(dllimport) int memcmp(const void* a, const void* b, size_t size);
__declspec(dllimport) const void* memchr(const void* buffer, int value, size_t buffer_size);
