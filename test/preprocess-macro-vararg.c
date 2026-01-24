#define f(a, ...) (a + sum(__VA_ARGS__))

static int sum(int x, int y, int z) {
	return x + y + z;
}

int main(void) {
	int v = f(5, 1, 2, 4);
	return v == 12 ? 0 : 1;
}
