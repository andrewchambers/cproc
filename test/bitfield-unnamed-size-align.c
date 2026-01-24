struct s {
	int : 8;
	char c;
};
union u {
	int : 8;
	char c;
};

int main(void) {
	int s1 = sizeof(struct s);
	int s2 = alignof(struct s);
	int u1 = sizeof(union u);
	int u2 = alignof(union u);
	if (s1 < (int)sizeof(char) || s2 < (int)alignof(char))
		return 1;
	if (u1 < (int)sizeof(char) || u2 < (int)alignof(char))
		return 1;
	return 0;
}
