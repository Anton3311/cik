#pragma once

__declspec(dllimport) size_t strlen(const char* string);
__declspec(dllimport) int strcmp(const char* a, const char* b);
__declspec(dllimport) int strncmp(const char* a, const char* b, size_t size);
