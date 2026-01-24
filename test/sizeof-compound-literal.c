int x = sizeof (int){1};

int main(void) {
	return x == (int)sizeof(int) ? 0 : 1;
}
