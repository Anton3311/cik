// This is a test of an arena implementation used through out the whole compiler. It is a bit
// trimmed down and simplified, to not include the asan and profiler related things.

#include <stdio.h>
#include <stdint.h>

__declspec(dllimport) void assert(uint64_t);
__declspec(dllimport) void panic(const char*);

// NOTE: The parse it not yet capable of parsing the Windows SDK headers, thus the needed
//       <memoryapi.h> functions and flags are defined here manually.

// Ignores the message
#define assert_msg(cond, msg) assert(cond)

typedef unsigned long DWORD;
typedef void* LPVOID;
typedef size_t SIZE_T;
typedef int BOOL;

#define MEM_COMMIT                      0x00001000  
#define MEM_RESERVE                     0x00002000  
#define MEM_RELEASE                     0x00008000  

#define PAGE_READWRITE          0x04    

__declspec(dllimport) LPVOID VirtualAlloc(
		LPVOID lpAddress,
		SIZE_T dwSize,
		DWORD flAllocationType,
		DWORD flProtect);

__declspec(dllimport) BOOL VirtualFree(
		LPVOID lpAddress,
		SIZE_T dwSize,
		DWORD dwFreeType);

#define NULL (void*)0

#define PAGE_SIZE 4096

typedef struct {
	size_t capacity;
	size_t commited;
	size_t allocated;
	uint8_t* base;
} Arena;

size_t align(size_t value, size_t alignment) {
	return (value + alignment - 1) / alignment * alignment;
}

size_t _compute_page_count(size_t bytes) {
	return (bytes + PAGE_SIZE - 1) / PAGE_SIZE;
}

void _arena_reserve(Arena* arena) {
	arena->base = (uint8_t*)VirtualAlloc(NULL,
			(SIZE_T)align(arena->capacity, PAGE_SIZE),
			MEM_RESERVE,
			PAGE_READWRITE);

	assert(arena->base != NULL);
}

void _arena_commit(Arena* arena, size_t size) {
	size_t page_count = _compute_page_count(size);

	size_t commit_size = page_count * PAGE_SIZE;
	if (arena->commited + commit_size > arena->capacity) {
		panic("Out of arena memory");
	}

	void* result = VirtualAlloc(arena->base + arena->commited,
			(SIZE_T)commit_size,
			MEM_COMMIT,
			PAGE_READWRITE);

	assert(result != NULL);

	arena->commited += commit_size;
}

void arena_release(Arena* arena) {
	if (arena->base == NULL) {
		return;
	}

	BOOL result = VirtualFree(arena->base, 0, MEM_RELEASE);
	assert_msg(result, "Failed to free arena memory");

	arena->base = NULL;
	arena->allocated = 0;
	arena->commited = 0;
}

void* arena_alloc_aligned(Arena* arena, size_t size, size_t alignment) {
	size_t allocation_base = align(arena->allocated, alignment);
	size_t new_allocated_ptr = allocation_base + size;

	if (arena->base == NULL) {
		_arena_reserve(arena);
	}

	if (new_allocated_ptr > arena->commited) {
		_arena_commit(arena, new_allocated_ptr - arena->allocated);
	}

	void* allocation = arena->base + allocation_base;
	arena->allocated = new_allocated_ptr;
	return allocation;
}

void arena_reset(Arena* arena) {
	arena->allocated = 0;
}

#define offsetof(type, field_name) ((size_t)(&((type*)0)->field_name))
#define alignof(type) offsetof(struct { char c; type field; }, field)

#define arena_alloc(arena, type) (type*)arena_alloc_aligned(arena, sizeof(type), alignof(type))
#define arena_alloc_array(arena, type, count) (type*)arena_alloc_aligned(arena, sizeof(type) * count, alignof(type))

#define sub_test(name) printf("%d: " name "\n", __LINE__);

int main(int argc, char *argv[]) {
	Arena arena = { .capacity = PAGE_SIZE * 2 };

	sub_test("simple allocation") {
		int* a = arena_alloc(&arena, int);
		*a = 100;

		assert(*a == 100);
	}

	sub_test("reset") {
		arena_reset(&arena);

		assert(arena.allocated == 0);
	}

	sub_test("fill first page") {
		arena_alloc_aligned(&arena, PAGE_SIZE, 1);

		assert(arena.allocated == PAGE_SIZE);
		assert(arena.commited == PAGE_SIZE);
	}

	sub_test("allocate into second page") {
		int* some = arena_alloc(&arena, int);
		*some = 88;

		assert(*some == 88);
		assert(arena.commited == PAGE_SIZE * 2);
	}

	sub_test("release") {
		arena_release(&arena);

		assert(arena.base == NULL);
		assert(arena.allocated == 0);
		assert(arena.commited == 0);
	}

	return 0;
}
