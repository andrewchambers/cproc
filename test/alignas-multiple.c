#include <stdint.h>

alignas(8) alignas(4) char x;

int main(void) {
	return ((uintptr_t)&x & 7) == 0 ? 0 : 1;
}
