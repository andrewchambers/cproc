int main(void) {
	int x[1] = {0}, *p = x;
	*p++ += 1;
	return (x[0] == 1 && p == x + 1) ? 0 : 1;
}
