void test0();
void test1() {}
void (test2)() {}

int main(int argc, char *argv[]) {
	int                    a0;
	int*                   a1;
	const int* const       a2;
	int* const             a3;
	const int* const      (a4);
	const int* const      (a5[4]);
	const int* const      (a6)[4];

	int (*a7)(void*, int);
	return 0;
}
