double f(double x) {
	return -x;
}

int main(void) {
	return f(1.5) == -1.5 ? 0 : 1;
}
