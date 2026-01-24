long double add(long double a, long double b) { return a + b; }
long double sub(long double a, long double b) { return a - b; }
long double mul(long double a, long double b) { return a * b; }
long double divv(long double a, long double b) { return a / b; }
int lt(long double a, long double b) { return a < b; }

int main(void) {
	if (sizeof(long double) != 16)
		return 1;
	if (_Alignof(long double) != 16)
		return 2;
	long double a = 1.5L;
	long double b = 2.25L;
	if (add(a, b) != 3.75L)
		return 3;
	if (sub(b, a) != 0.75L)
		return 4;
	if (mul(a, b) != 3.375L)
		return 5;
	if (divv(b, a) != 1.5L)
		return 6;
	if (!lt(a, b))
		return 7;
	if (lt(b, a))
		return 8;
	return 0;
}
