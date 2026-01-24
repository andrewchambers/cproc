struct pair {
	int x, y;
};

struct pair g(void) {
	return (struct pair){1, 7};
}

int f(void) {
	return g().y;
}

int main(void) {
	return f() == 7 ? 0 : 1;
}
