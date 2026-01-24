#define stringize(a) #a

int main(void) {
	const char *s = stringize(hello);
	return (s[0] == 'h' && s[4] == 'o' && s[5] == 0) ? 0 : 1;
}
