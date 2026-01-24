/* C11 6.10.3.4p4 */
#define f(a) a*g
#define g(a) f(a)

int main(void) {
	int g = 7;
	int v = f(2)(9);
	return v == 2 * 9 * g ? 0 : 1;
}
