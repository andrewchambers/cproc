int main(void) {
	int x = 0;
	volatile int *p = &x;
	*p = 5;
	int y = *p;
	return y == 5 ? 0 : 1;
}
