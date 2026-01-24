union {
	int x;
	double y;
} a = {.x = 5}, b = {.y = 7.5};

int main(void) {
	if (a.x != 5)
		return 1;
	return b.y == 7.5 ? 0 : 1;
}
