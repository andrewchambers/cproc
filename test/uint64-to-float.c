unsigned long long g(void) {
	return 16777217ull; /* 2^24 + 1 */
}

float f(void) {
	return g();
}

int main(void) {
	/* float can't represent +1 at this magnitude, expect round to 2^24 */
	return f() == 16777216.0f ? 0 : 1;
}
