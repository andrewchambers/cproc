float g(void) {
	return 12345.9f;
}

unsigned long long f(void) {
	return g();
}

int main(void) {
	return f() == 12345ull ? 0 : 1;
}
