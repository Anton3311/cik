#pragma once

#define STD_IMPORT __declspec(dllimport)

__declspec(dllimport) int printf(const char* fmt, ...);

typedef struct FILE FILE;

__declspec(dllimport) FILE* fopen(const char* path,
		const char* mode);

__declspec(dllimport) void fclose(FILE* file);

__declspec(dllimport) void fread(void* buffer,
		size_t element_size,
		size_t element_count,
		FILE* file);

__declspec(dllimport) size_t fwrite(void const* buffer,
		size_t element_size,
		size_t element_count,
		FILE* file);

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

__declspec(dllimport) int fseek(FILE* file, long offset, int origin);
__declspec(dllimport) long ftell(FILE* file);

