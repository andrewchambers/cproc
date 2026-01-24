#define str(x) #x

int main(void) {
	const char *a = str(@);
	const char *b = str(12 x-    3
 abc);
	const char *c = str('"' "\\");
	const char *d = str( abc);

	int ok = 1;
	ok &= (a[0] == '@' && a[1] == 0);
	ok &= (b[0] == '1' && b[1] == '2');
	ok &= (d[0] == 'a' && d[1] == 'b' && d[2] == 'c' && d[3] == 0);
	ok &= (c[0] == '\'' && c[1] == '"');

	return ok ? 0 : 1;
}
