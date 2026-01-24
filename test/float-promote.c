static int sink;

void g1(int x, ...) {
	sink = x;
}

void g2(float f) {
	sink = (int)f;
}

int main(void) {
	g1(0, 1.0f);
	if (sink != 0)
		return 1;
	g2(1.0f);
	return sink == 1 ? 0 : 1;
}
