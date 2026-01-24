#define f(a, b) ((a) + abc + (b))

int main(void) {
	int abc = 3;
	int foo = 4;
	int bar = 5;

	return f(foo, bar) == 12 ? 0 : 1;
}
