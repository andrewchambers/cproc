int x[2][3];

int main(void) {
	x[1][2] = 5;
	return x[1][2] == 5 ? 0 : 1;
}
