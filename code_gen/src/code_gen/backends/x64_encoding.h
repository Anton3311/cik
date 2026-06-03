#ifndef X64_ENCODING_H
#define X64_ENCODING_H

#include "core/core.h"

//
// CodeBuffer
//

typedef struct {
	uint8_t* buffer;
	size_t size;
	size_t capacity;
	Arena* allocator;
} CodeBuffer;

void code_buffer_init(CodeBuffer* buffer, Arena* allocator);
void code_buffer_grow(CodeBuffer* buffer, size_t expected_capacity);

inline uint8_t* code_buffer_append(CodeBuffer* buffer, size_t byte_count) {
	if (buffer->size + byte_count > buffer->capacity) {
		code_buffer_grow(buffer, buffer->size + byte_count);
	}

	uint8_t* bytes = buffer->buffer + buffer->size;
	buffer->size += byte_count;
	return bytes;
}

inline void code_buffer_push_64(CodeBuffer* buffer, uint64_t value) {
	uint8_t* a = code_buffer_append(buffer, sizeof(value));
	a[7] = (uint8_t)(value >> 56);
	a[6] = (uint8_t)((value >> 48) & 0xff);
	a[5] = (uint8_t)((value >> 40) & 0xff);
	a[4] = (uint8_t)((value >> 32) & 0xff);
	a[3] = (uint8_t)((value >> 24) & 0xff);
	a[2] = (uint8_t)((value >> 16) & 0xff);
	a[1] = (uint8_t)((value >> 8) & 0xff);
	a[0] = (uint8_t)((value >> 0) & 0xff);
}

inline void code_buffer_push_32(CodeBuffer* buffer, uint32_t value) {
	uint8_t* a = code_buffer_append(buffer, sizeof(value));
	a[3] = (uint8_t)((value >> 24) & 0xff);
	a[2] = (uint8_t)((value >> 16) & 0xff);
	a[1] = (uint8_t)((value >> 8) & 0xff);
	a[0] = (uint8_t)((value >> 0) & 0xff);
}

inline void code_buffer_push_8(CodeBuffer* buffer, uint8_t value) {
	uint8_t* a = code_buffer_append(buffer, sizeof(value));
	*a = value;
}

#endif
