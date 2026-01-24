struct {
	unsigned : 30, a : 2, b;
} s = {5};

int main(void) {
	return (s.a == 1 && s.b == 0) ? 0 : 1;
}
