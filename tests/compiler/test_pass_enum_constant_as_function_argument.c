typedef enum {
	CONST_A,
	CONST_B
} Enum;

void func(Enum e) {

}

int main(int argc, char *argv[]) {
	func(CONST_A);
	return 0;
}
