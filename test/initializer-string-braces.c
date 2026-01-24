char s[] = {"abc"};

int main(void) {
	return (sizeof s == 4 &&
		s[0] == 'a' &&
		s[1] == 'b' &&
		s[2] == 'c' &&
		s[3] == '\0') ? 0 : 1;
}
