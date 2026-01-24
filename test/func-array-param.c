typedef int T1[];
typedef const int T2[];

void f(int a[const], const int b[], const T1 c, T2 d) {
	static_assert(__builtin_types_compatible_p(typeof(&a), int *const*));
	static_assert(__builtin_types_compatible_p(typeof(&b), const int **));
	static_assert(__builtin_types_compatible_p(typeof(&c), const int **));
	static_assert(__builtin_types_compatible_p(typeof(&d), const int **));
}

int main(void) {
	f(0, 0, 0, 0);
	return 0;
}
