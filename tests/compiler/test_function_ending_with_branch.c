// Test that the compiler can properly insert the implicit `return void`, without hitting an assert
void branch(int a) {
	if (a) {

	}
}

int main(int argc, char *argv[]) {
	return 0;
}
