typedef int T[2];

int main(void) {
	int ok = 1;
	ok &= __builtin_types_compatible_p(int[2], int[1 + 1]);
	ok &= !__builtin_types_compatible_p(int[2], int[1]);
	ok &= !__builtin_types_compatible_p(int[2], unsigned[2]);
	ok &= !__builtin_types_compatible_p(const int (*)[2], int (*)[2]);
	ok &= __builtin_types_compatible_p(float[], float[3]);
	return ok ? 0 : 1;
}
