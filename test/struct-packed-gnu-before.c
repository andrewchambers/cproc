__attribute__((packed)) struct Q {
	char a;
	int b;
};

static_assert(__builtin_offsetof(struct Q, b) == 1);
static_assert(sizeof(struct Q) == 5);

int main(void)
{
	return 0;
}
