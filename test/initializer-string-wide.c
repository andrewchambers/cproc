typedef typeof(L' ') wide_t;

static const char s[] = "ab";
static const unsigned char u8[] = u8"a\xCE\xB1";
static const unsigned short u[] = u"a\x03B1";
static const unsigned U[] = U"a\x03B1";
static const wide_t W[] = L"a\x03B1";

int main(void) {
	int ok = 1;

	ok &= (s[0] == 'a');
	ok &= (s[1] == 'b');
	ok &= (s[2] == 0);

	ok &= (u8[0] == (unsigned char)'a');
	ok &= ((unsigned char)u8[1] == 0xCE);
	ok &= ((unsigned char)u8[2] == 0xB1);
	ok &= (u8[3] == 0);

	ok &= (u[0] == (unsigned short)'a');
	ok &= (u[1] == 0x03B1);
	ok &= (u[2] == 0);

	ok &= (U[0] == (unsigned)'a');
	ok &= (U[1] == 0x03B1);
	ok &= (U[2] == 0);

	ok &= (W[0] == (wide_t)L'a');
	ok &= (W[1] == (wide_t)0x03B1);
	ok &= (W[2] == 0);

	return ok ? 0 : 1;
}
