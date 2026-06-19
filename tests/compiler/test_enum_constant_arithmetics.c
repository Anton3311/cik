void assert(unsigned long long);

typedef enum {
	zero,
	one,
	two,
} Nums;

int main(int argc, char* argv[]) {
	assert(zero == 0);
	assert(one == 1);
	assert(two == 2);

	assert(zero + 1 == 1);
	assert(zero + one == 1);
	assert(two + one == 3);
	return 0;
}
