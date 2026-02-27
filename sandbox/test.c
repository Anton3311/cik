#define unreachable __debugbreak()

int main() {
	unreachable;
	return 0;
}
