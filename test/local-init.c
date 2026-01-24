int main(void) {
	struct {
		char c[8];
		long long i[3];
	} s = {'a'};

	if (s.c[0] != 'a' || s.c[1] != 0)
		return 1;
	if (s.i[0] != 0 || s.i[1] != 0 || s.i[2] != 0)
		return 1;
	return 0;
}
