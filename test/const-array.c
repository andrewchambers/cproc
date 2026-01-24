/* C11 6.7.3p9 - type qualifiers on array type qualify the element type */
typedef int T[2];

void f(const T x) {
	(void)x;
}

int main(void) {
	int a[2] = {1, 2};
	f(a);
	return a[0] == 1 && a[1] == 2 ? 0 : 1;
}
