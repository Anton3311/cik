__declspec(dllimport) void panic(const char*);

int main(int argc, char* argv[]) {
	if (10) {
		if (9) {
			return 0;
		}
	}

	panic("Unreachable");
	return 0;
}
