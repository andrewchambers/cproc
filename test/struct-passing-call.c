struct s {
	int x;
} s;

static int seen;

void f(struct s v) {
	seen = v.x;
}

void g(void) {
	f(s);
}

int main(void) {
	s.x = 7;
	g();
	return seen == 7 ? 0 : 1;
}
