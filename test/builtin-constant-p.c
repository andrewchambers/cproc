int f(void);
int x = __builtin_constant_p(1 + 2 * 3);
int y = __builtin_constant_p(f());

int main(void) {
	return (x == 1 && y == 0) ? 0 : 1;
}
