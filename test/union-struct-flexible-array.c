union u {
	struct {
		int a;
		short b[];
	} s;
	unsigned char storage[32];
};

int x = sizeof(union u);

int main(void) {
	union u u = {0};
	u.s.a = 5;
	return (x == 32 && u.s.a == 5) ? 0 : 1;
}
