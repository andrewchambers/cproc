union u {
	int x;
	float y;
};

int f(union u u, int x)
{
	return x;
}
