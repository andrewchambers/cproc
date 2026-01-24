#include <stdint.h>

alignas(32) long x[4] = {1, 2, 3, 4};

int main(void) {
	if (((uintptr_t)x & 31) != 0)
		return 1;
	return (x[0] == 1 && x[3] == 4) ? 0 : 1;
}
