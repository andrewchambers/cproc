#define foo foo + 1
#undef foo
int foo = 3;
#define foo foo + 1

int main(void) {
	return foo == 4 ? 0 : 1;
}
