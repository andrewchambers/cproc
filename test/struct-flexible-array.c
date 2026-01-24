#include <stdlib.h>

struct s {
	int a;
	short b[];
};

int main(void) {
	size_t n = 4;
	struct s *p = malloc(sizeof(*p) + n * sizeof(short));
	int ok = 1;

	if (!p)
		return 1;

	p->a = 7;
	for (size_t i = 0; i < n; i++)
		p->b[i] = (short)(i + 1);

	ok &= (sizeof(struct s) == sizeof(int));
	ok &= (p->a == 7);
	ok &= (p->b[2] == 3);

	free(p);
	return ok ? 0 : 1;
}
