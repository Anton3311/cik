__declspec(dllimport) void assert(unsigned long long);

struct Type {
	char c;
	int i32;
	short i16;
	void* ptr;
};

typedef struct Type Type;

#define offsetof(type, field_name) ((size_t)(&((type*)0)->field_name))

int main(int argc, char* argv[]) {
	assert(offsetof(Type, c) == 0);
	assert(offsetof(Type, i32) == 4);
	assert(offsetof(Type, i16) == 8);
	assert(offsetof(Type, ptr) == 16);
	return 0;
}
