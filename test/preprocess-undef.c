#define FOO 3
#undef FOO

#ifndef FOO
#define FOO 9
#endif

int main(void) {
	return FOO == 9 ? 0 : 1;
}
