struct {
	int x;
} s = {
	.x = 3,
};

int *p = &s.x;

int main(void) {
	return *p == 3 ? 0 : 1;
}
