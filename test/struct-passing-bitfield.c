struct s {
	char x, y;
	long long z : 48;
};

static int seen;

void f(struct s v) {
	seen = (int)v.z;
}

int main(void) {
	struct s v = {'a', 'b', 1234};
	f(v);
	return seen == 1234 ? 0 : 1;
}
