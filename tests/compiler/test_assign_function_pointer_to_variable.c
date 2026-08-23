void callback_impl(int) {

}

int main(int argc, char *argv[]) {
	void(*callback)(int) = callback_impl;
	return 0;
}
