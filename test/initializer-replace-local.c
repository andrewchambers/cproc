int main(void) {
	struct {
		char s[6];
	} x = {
		.s[0] = 'x',
		.s[4] = 'y',
		.s = "hello",
		.s[1] = 'a',
	};
	return (x.s[0] == 'h' && x.s[1] == 'a' && x.s[4] == 'o') ? 0 : 1;
}
