const struct {
	struct {
		int x, y;
	} t;
} s = {{{1}, 2}};

int main(void) {
	return (s.t.x == 1 && s.t.y == 2) ? 0 : 1;
}
