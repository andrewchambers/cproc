typedef int *t;
const typeof(t) y = 0;
int *const y;

int main(void) {
	return y == 0 ? 0 : 1;
}
