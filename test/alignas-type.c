#include <stdint.h>

alignas(int) char x[4];

int main(void) {
	return ((uintptr_t)x % _Alignof(int)) == 0 ? 0 : 1;
}
