int x = __builtin_types_compatible_p(unsigned, enum {A});
int y = __builtin_types_compatible_p(const int, int);
int z = __builtin_types_compatible_p(int *, unsigned *);

int main(void) {
	return (x == 1 && y == 1 && z == 0) ? 0 : 1;
}
