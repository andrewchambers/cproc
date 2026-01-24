struct s {
	float x;
	long y;
};

struct s s = {};

static int local_zero(void) {
	struct s t = {};
	return t.x == 0.0f && t.y == 0;
}

int main(void) {
	return (s.x == 0.0f && s.y == 0 && local_zero()) ? 0 : 1;
}
