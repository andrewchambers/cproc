long double from_i64(long long v) { return v; }
long long to_i64(long double v) { return (long long)v; }

int main(void) {
	long long v = 1234567890123LL;
	long double ld = from_i64(v);
	if (to_i64(ld) != v)
		return 1;
	if (to_i64(1234.75L) != 1234)
		return 2;
	if (to_i64(-1234.75L) != -1234)
		return 3;
	return 0;
}
