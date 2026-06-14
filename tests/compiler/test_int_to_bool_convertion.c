void panic(const char* message);

int main(int argc, char* argv[]) {
	if (-10) {
		panic("A negative number must convert to`false`");
	}

	if (0) {
		panic("A zero must convert to`false`");
	}

	if (10) {
	} else {
		panic("A positive number must convert to`false`");
	}

	return 0;
}
