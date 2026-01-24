int copy_first(int n, ...) {
	__builtin_va_list ap;
	__builtin_va_list bp;
	int a;
	int b;

	__builtin_va_start(ap, n);
	__builtin_va_copy(bp, ap);
	a = __builtin_va_arg(ap, int);
	b = __builtin_va_arg(bp, int);
	__builtin_va_end(ap);
	__builtin_va_end(bp);
	return a == b;
}

int main(void) {
	return copy_first(0, 7, 9) ? 0 : 1;
}
