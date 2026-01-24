#include <stdint.h>

alignas(8) char c;

int main(void) {
	return ((uintptr_t)&c & 7) == 0 ? 0 : 1;
}
