int a[4];
int *p = a + 2 - 1;

int main(void) {
	return p == &a[1] ? 0 : 1;
}
