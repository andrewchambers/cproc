#include <stdarg.h>

long double sum(int n, ...) {
	va_list ap;
	long double s = 0.0L;
	va_start(ap, n);
	for (int i = 0; i < n; i++)
		s += va_arg(ap, long double);
	va_end(ap);
	return s;
}

int main(void) {
	long double s = sum(3, 1.0L, 2.0L, 3.0L);
	return s == 6.0L ? 0 : 1;
}
