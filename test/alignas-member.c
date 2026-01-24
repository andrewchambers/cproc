#include <stdint.h>

struct {
	int x;
	alignas(32) char s[32];
} s = {123, "abc"};

int main(void) {
	if (((uintptr_t)s.s & 31) != 0)
		return 1;
	return (s.x == 123 && s.s[0] == 'a' && s.s[1] == 'b' && s.s[2] == 'c') ? 0 : 1;
}
