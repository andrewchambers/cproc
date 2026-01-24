float g(void) {
	return 3.25f;
}

unsigned f(void) {
	return g();
}

int main(void) {
	return f() == 3u ? 0 : 1;
}
