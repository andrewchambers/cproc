struct s {
	int x;
};

static void
set(struct s v)
{
	v.x = 5;
}

int
main(void)
{
	struct s v = {1};
	set(v);
	return v.x == 1 ? 0 : 1;
}
