int main(void) {
	char s[10] = "abc";
	return (s[0] == 'a' && s[1] == 'b' && s[2] == 'c' &&
		s[3] == '\0' && s[4] == 0 && s[9] == 0) ? 0 : 1;
}
