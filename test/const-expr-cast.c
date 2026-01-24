enum {
	A = (unsigned char)0x321,
	B = (short)-2147438112,
	C = 0x80000003 * 2,
};

int a = A, b = B, c = C;

int main(void) {
	if (a != (unsigned char)0x321)
		return 1;
	if (b != (short)-2147438112)
		return 1;
	if (c != 0x80000003 * 2)
		return 1;
	return 0;
}
