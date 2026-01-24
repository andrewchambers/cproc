int x[2];

int main(void) {
	return (1 + x == &x[1]) ? 0 : 1;
}
