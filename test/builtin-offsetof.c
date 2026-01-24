struct s {
	struct {
		union {
			float z;
			char *c;
		} b;
	} a[5];
};
int x = __builtin_offsetof(struct s, a[2].b.c);

int main(void) {
	int a0 = (int)__builtin_offsetof(struct s, a);
	int elem = (int)sizeof(((struct s *)0)->a[0]);
	int inner = (int)__builtin_offsetof(typeof(((struct s *)0)->a[0]), b.c);
	return x == a0 + 2 * elem + inner ? 0 : 1;
}
