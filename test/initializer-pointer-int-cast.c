int x;
long p = (long)&x;

int main(void) {
	return p == (long)&x ? 0 : 1;
}
