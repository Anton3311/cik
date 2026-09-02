__declspec(dllimport) void assert(unsigned long long);

void early_return(int a) {
	switch (a) {
	case 1:
		assert(0);
	case 2:
		assert(0);
	case 3:
		assert(1);
		return;
	case 4:
		assert(0);
	case 5:
		assert(0);
	}
}

int main(int argc, char *argv[]) {
	early_return(3);
	return 0;
}
