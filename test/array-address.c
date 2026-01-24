int x[2], y = sizeof(*&x);

int main(void) {
	return y == (int)sizeof x ? 0 : 1;
}
