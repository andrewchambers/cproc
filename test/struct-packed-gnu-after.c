struct P {
	char a;
	int b;
} __attribute__((packed));

static_assert(__builtin_offsetof(struct P, b) == 1);
static_assert(sizeof(struct P) == 5);

int main(void)
{
	return 0;
}
