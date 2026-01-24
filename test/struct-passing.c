struct s {
	int x;
	struct {
		char y[3];
		short z;
	} s[2];
	double w;
};

static int seen;

void f(struct s v) {
	seen = v.x + v.s[1].z;
}

int main(void) {
	struct s v = {5, {{'a','b','c', 2}, {'d','e','f', 3}}, 1.0};
	f(v);
	return seen == 8 ? 0 : 1;
}
