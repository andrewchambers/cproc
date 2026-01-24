struct {
	unsigned short s[6];
} u = {
	.s = u"hello",
	.s[2] = u'£',
};

struct {
	unsigned s[5];
} U = {
	.s = U"hello",
	.s[3] = U'😃',
};

struct {
	__typeof__(L' ') s[5];
} L = {
	.s = L"hello",
	.s[3] = L'😃',
};

int main(void) {
	return (u.s[0] == u'h' && u.s[1] == u'e' && u.s[2] == u'£' &&
		U.s[0] == U'h' && U.s[3] == U'😃' &&
		L.s[0] == L'h' && L.s[3] == L'😃') ? 0 : 1;
}
