__declspec(dllimport) void panic(const char*);

int main(int argc, char* argv[]) {
	if (0) {
	} else {
		return 0;
	}

	panic("Unreachable");
	return 0;
}
