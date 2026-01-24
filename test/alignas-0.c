#include <stdint.h>

alignas(0) int x;
alignas(8) alignas(0) int y;
alignas(0) alignas(16) int z;

int main(void) {
	if (((uintptr_t)&x % _Alignof(int)) != 0)
		return 1;
	if (((uintptr_t)&y % 8) != 0)
		return 1;
	if (((uintptr_t)&z % 16) != 0)
		return 1;
	return 0;
}
