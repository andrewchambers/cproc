static int last;

void g(int v) {
	last = v;
}

int main(void) {
	static const unsigned char c = 0;
	g(c);
	if (last != 0)
		return 1;
	g(~c);
	if (last != -1)
		return 1;
	return 0;
}
