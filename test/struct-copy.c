struct s {
	char s[5];
	float f;
} x = {"hey", 2.0f};

int main(void) {
	struct s y = x;
	return (y.s[0] == 'h' && y.s[1] == 'e' && y.s[2] == 'y' &&
		y.s[3] == '\0' && y.f == 2.0f) ? 0 : 1;
}
