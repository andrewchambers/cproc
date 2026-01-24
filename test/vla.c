short g() { return 1; }
long f(void) {
    double a[10 + g()];
    return sizeof(a);
}

int main(void) {
	return f() == (long)sizeof(double) * 11 ? 0 : 1;
}
