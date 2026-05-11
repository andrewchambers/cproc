long double global = 1.5L;
static long double table[2] = {2.0L, -0.5L};

long double
identity(long double x)
{
	return x;
}

long double
arith(long double a, long double b)
{
	return (a + b) * (a - b) / (a ? a : 1.0L);
}

long double
mixed(int a, int b, int c, int d, int e, int f, int g, long double x, long double y)
{
	return x + y + (long double)g;
}

long double
callmixed(void)
{
	return mixed(1, 2, 3, 4, 5, 6, 7, global, table[0]) + table[1];
}

int
compare(long double a, long double b)
{
	return a < b || a <= b || b > a || b >= a || a == b || a != b;
}

int
convert(long double x)
{
	return (int)x + ((long double)7 == 7.0L) + (x ? 1 : 0);
}

int
precision(void)
{
	return 0x1.000000000000001p0L != 1.0L;
}
