static int sum;

void g(int v) {
	sum += v;
}

void f(void) {
	int i;
	for (i = 0; i < 10; ++i)
		g(i);
}

int main(void) {
	f();
	return sum == 45 ? 0 : 1;
}
