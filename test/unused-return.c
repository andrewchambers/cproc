int g(void) {
	return 5;
}

void f(void) {
	g();
}

int main(void) {
	f();
	return 0;
}
