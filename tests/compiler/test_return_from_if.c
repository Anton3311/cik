extern void panic(const char*);

int main(int argc, char* argv[]) {
	if (10) {
		return 0;
	}

	panic("Unreachable");
	return 0;
}
