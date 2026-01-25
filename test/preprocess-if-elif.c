#define A 3
#define B 4

#if defined(A) && A == 3
#define V 1
#elif defined(B) && B == 4
#define V 2
#else
#define V 3
#endif

#if 0
#error should not fire
#endif

int main(void) {
	return V == 1 ? 0 : 1;
}
