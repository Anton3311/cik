typedef struct {
	const char* v;
	size_t length;
} String;

#define STR_LIT(string) (String) { .v = string, .length = sizeof(string) - 1 }

__declspec(dllimport) void assert(unsigned long long);

int main(int argc, char *argv[]) {
	String s = STR_LIT("Hello world");
	assert(s.length == 11);
	return 0;
}
