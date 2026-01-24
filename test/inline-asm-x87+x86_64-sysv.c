int main(void)
{
	long double x = 5.5L;
	long double y = 2.0L;
	unsigned short fpsr = 0;
	__asm__ ("fprem; fnstsw %%ax" : "+t"(x), "=a"(fpsr) : "u"(y));
	if (x < 1.499L || x > 1.501L)
		return 1;
	return 0;
}
