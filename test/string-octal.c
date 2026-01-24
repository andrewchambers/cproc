static const char s[] = "\1\12\123\1234";

int main(void) {
	return ((unsigned char)s[0] == 1 &&
		(unsigned char)s[1] == 10 &&
		(unsigned char)s[2] == 83 &&
		(unsigned char)s[3] == 83 &&
		s[4] == '4' &&
		s[5] == '\0') ? 0 : 1;
}
