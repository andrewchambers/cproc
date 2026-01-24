int x[2][3];
int *y = &x[1][2];

int main(void) {
	return y == &x[1][2] ? 0 : 1;
}
