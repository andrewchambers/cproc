/* C11 6.10.3.5p5 with token concatenation disabled for now */
#define    x          3
#define    f(a)       f(x * (a))
#undef     x
#define    x          2
#define    g          f
#define    z          z[0]
#define    h          g(~
#define    m(a)       a(w)
#define    w          0,1
#define    t(a)       a
#define    p()        int
#define    q(x)       x
//#define    r(x,y)     x ## y
#define    str(x)     # x

p() i[q()] = { q(1) };
char c[2][6] = { str(hello), str() };

int main(void) {
	int ok = 1;

	ok &= (i[0] == 1);
	ok &= (c[0][0] == 'h');
	ok &= (c[0][5] == 0);
	ok &= (c[1][0] == 0);

	return ok ? 0 : 1;
}
