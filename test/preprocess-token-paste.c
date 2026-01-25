#define CAT(a, b) a##b
#define XCAT(a, b) CAT(a, b)

int main(void) {
	int foobar = 7;
	return XCAT(foo, bar) == 7 ? 0 : 1;
}
