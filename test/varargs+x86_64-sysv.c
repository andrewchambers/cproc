int first(int n, ...) {
	__builtin_va_list ap;
	int v;

	__builtin_va_start(ap, n);
	v = __builtin_va_arg(ap, int);
	__builtin_va_end(ap);
	return v;
}

int sum3(int n, ...) {
	__builtin_va_list ap;
	int a;
	int b;
	int c;

	__builtin_va_start(ap, n);
	a = __builtin_va_arg(ap, int);
	b = __builtin_va_arg(ap, int);
	c = __builtin_va_arg(ap, int);
	__builtin_va_end(ap);
	return n + a + b + c;
}

int main(void) {
	int ok = 1;

	ok &= (first(0, 7) == 7);
	ok &= (sum3(1, 2, 3, 4) == 10);

	return ok ? 0 : 1;
}
