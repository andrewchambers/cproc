float x = __builtin_inff();

int main(void) {
	return x > 1e30f ? 0 : 1;
}
