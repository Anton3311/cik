__declspec(dllimport) void assert(unsigned long long);

void case_1(int a) {
	switch (a) {
	case 0:
		assert(1);
	}
}

void case_2(int a) {
	switch (a) {
	case 1:
		assert(0);
	case 2:
		assert(1);
	}
}

void fallthrough_1(int a) {
	switch (a) {
	case 1:
		assert(0);
	case 2:
		assert(1);
	case 3:
		assert(1);
	case 4:
		assert(1);
	case 5:
		assert(1);
	}
}

void case_none(int a) {
	switch (a) {
	case 1:
		assert(0);
	case 2:
		assert(0);
	case 3:
		assert(0);
	case 4:
		assert(0);
	}
}

int main(int argc, char *argv[]) {
	case_1(1);
	case_2(2);

	fallthrough_1(2);
	fallthrough_1(3);
	fallthrough_1(4);
	fallthrough_1(5);

	case_none(5);
	return 0;
}
