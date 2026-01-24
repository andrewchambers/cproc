int x[5];
typeof(x) y;

int main(void) {
	return (sizeof y == sizeof x) ? 0 : 1;
}
