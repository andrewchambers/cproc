int *x = &(int){2};

int main(void) {
	return *x == 2 ? 0 : 1;
}
