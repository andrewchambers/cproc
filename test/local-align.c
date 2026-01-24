#include <stdint.h>

void *addr;

void f(void) {
	alignas(16) char x[4];
	addr = x;
}

int main(void) {
	f();
	return ((uintptr_t)addr & 15) == 0 ? 0 : 1;
}
