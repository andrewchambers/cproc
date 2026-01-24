#define g(x) x
#define f(a) g a (3)

static int plus(int x) {
	return x + 1;
}

int main(void) {
	int ok = 1;

	ok &= (f((plus)) == 4);
	ok &= (f() == 3);

	return ok ? 0 : 1;
}
