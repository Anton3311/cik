void test0();
void test1() {}
void (test2)() {}

void test3(int a[], const int* b, const int* const c, const int* d[5]) {

}

void test4(void(*callback)(int a, int b)) {

}

struct Struct {
	int                    a0;
	int*                   a1;
	const int* const       a2;
	int* const             a3;
	const int* const      (a4);
	const int* const      (a5[4]);
	const int* const      (a6)[4];

	int (*a7)(void*, int);
};

typedef int                    Type0;
typedef int*                   Type1;
typedef const int* const       Type2;
typedef int* const             Type3;
typedef const int* const      (Type4);
typedef const int* const      (Type5[4]);
typedef const int* const      (Type6)[4];

typedef int (*Type7)(void*, int);

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
