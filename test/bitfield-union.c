union u {
	struct {
		unsigned x : 4;
		unsigned y : 4;
	} b;
	unsigned v;
};

int main(void) {
	union u u = {0};
	u.b.x = 5;
	u.b.y = 10;
	return ((u.v & 0xF) == 5 && ((u.v >> 4) & 0xF) == 10) ? 0 : 1;
}
