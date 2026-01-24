int main(void)
{
	int x = 0;
	__asm__ volatile ("mov $5, %0" : "=r"(x));
	if (x != 5)
		return 1;

	int y = 3;
	int z = 0;
	__asm__ ("mov %1, %0" : "=r"(z) : "r"(y));
	if (z != 3)
		return 2;

	int m = 1;
	__asm__ volatile ("incl %0" : "+m"(m));
	if (m != 2)
		return 3;

	int a = 7;
	int b = 0;
	__asm__ ("add $5, %0" : "=r"(b) : "0"(a));
	if (b != 12)
		return 4;

	long p = 3;
	long q = 4;
	long r = 0;
	__asm__ ("lea (%1,%2), %0" : "=D"(r) : "S"(p), "d"(q));
	if (r != 7)
		return 5;

	float f = 4.0f;
	__asm__ ("sqrtss %1, %0" : "=x"(f) : "x"(f));
	if (f < 1.999f || f > 2.001f)
		return 6;

	int qv = 1;
	__asm__ ("" : "+r"(qv) : : "memory");
	if (qv != 1)
		return 7;

	int w = 2;
	__asm__ __volatile__ ("" : : "r"(w) : "memory");
	if (w != 2)
		return 8;

	int w2 = 3;
	__asm__ __volatile ("" : : "r"(w2) : "memory");
	if (w2 != 3)
		return 9;

	return 0;
}
