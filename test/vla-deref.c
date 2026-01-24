int main(void) {
	int l = 3;
	char a[*&l];
	return sizeof a == 3 ? 0 : 1;
}
