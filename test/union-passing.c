union u {
	int x;
	float y;
};

void f(union u u) {
	(void)u;
}

int main(void) {
	union u u = {.x = 3};
	f(u);
	return u.x == 3 ? 0 : 1;
}
