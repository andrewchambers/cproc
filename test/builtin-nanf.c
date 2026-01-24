float x = __builtin_nanf("");

int main(void) {
	return x != x ? 0 : 1;
}
