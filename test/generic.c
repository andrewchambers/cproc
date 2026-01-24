int x = _Generic(123,
	const int: 1,
	unsigned: 2,
	int: 3,
	int *: 4
);

int main(void) {
	return x == 3 ? 0 : 1;
}
