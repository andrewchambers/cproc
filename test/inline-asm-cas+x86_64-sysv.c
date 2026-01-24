static int cas(int *p, int t, int s)
{
	__asm__ __volatile__(
		"lock; cmpxchg %3, %1"
		: "=a"(t), "=m"(*p)
		: "a"(t), "r"(s)
		: "memory");
	return t;
}

int main(void)
{
	int x = 5;
	int r = cas(&x, 5, 7);
	if (r != 5 || x != 7)
		return 1;
	r = cas(&x, 5, 9);
	if (r != 7 || x != 7)
		return 2;
	return 0;
}
