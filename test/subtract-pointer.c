int f(int *x, int *y) {
	return (int)(x - y);
}

int main(void) {
	int a[4];
	return f(&a[3], &a[1]) == 2 ? 0 : 1;
}
