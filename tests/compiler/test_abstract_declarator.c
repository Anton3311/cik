int main(int argc, char *argv[]) {
	sizeof(int);
	sizeof(int*);
	sizeof(const int* const);
	sizeof(int* const);
	sizeof(const int* const);
	sizeof(const int* const[4]);
	sizeof(const int* (*)[4]);
	sizeof(int (*)(void*, int));
	sizeof(int (*[10])(void*, int));
	sizeof(int (*const [])(void*, int, ...));
	return 0;
}
