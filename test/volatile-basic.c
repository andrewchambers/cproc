volatile int g;

int main(void) {
	g = 7;
	return g == 7 ? 0 : 1;
}
