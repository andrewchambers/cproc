struct {
	char s[6];
} x = {
	.s = "hello",
	.s[1] = 'a',
};

int main(void) {
	return (x.s[0] == 'h' &&
		x.s[1] == 'a' &&
		x.s[2] == 'l' &&
		x.s[3] == 'l' &&
		x.s[4] == 'o' &&
		x.s[5] == '\0') ? 0 : 1;
}
