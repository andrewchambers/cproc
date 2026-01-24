int x;

int f(void) {
	return sizeof (x)++;
}

int main(void) {
	return f() == (int)sizeof x ? 0 : 1;
}
