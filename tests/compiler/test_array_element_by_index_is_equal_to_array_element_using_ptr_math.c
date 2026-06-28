void assert(unsigned long long);

int main(int argc, char* argv[]) {
	assert(*argv == argv[0]);
	return 0;
}
