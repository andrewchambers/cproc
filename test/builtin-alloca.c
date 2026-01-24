int main(void) {
	char *x = __builtin_alloca(32);
	x[0] = 1;
	x[31] = 2;
	return (x[0] == 1 && x[31] == 2) ? 0 : 1;
}
