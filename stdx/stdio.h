#pragma once

int printf(const char* fmt, ...);

typedef struct FILE FILE;

FILE* fopen(const char* path, const char* mode);
void fclose(FILE* file);

void fread(void* buffer, size_t element_size, size_t element_count, FILE* file);
size_t fwrite(void const* buffer, size_t element_size, size_t element_count, FILE* file);

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

int fseek(FILE* file, long offset, int origin);
long ftell(FILE* file);

