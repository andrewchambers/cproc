struct s {
	int x;
	static_assert(1, "");
};

int main(void) {
	struct s s = {1};
	return s.x == 1 ? 0 : 1;
}
