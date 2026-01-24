char x[] = "hello";

int main(void) {
	char y[] = "hello";
	return (x[0] == 'h' && x[1] == 'e' && x[2] == 'l' &&
		x[3] == 'l' && x[4] == 'o' && x[5] == '\0' &&
		y[0] == 'h' && y[1] == 'e' && y[2] == 'l' &&
		y[3] == 'l' && y[4] == 'o' && y[5] == '\0') ? 0 : 1;
}
