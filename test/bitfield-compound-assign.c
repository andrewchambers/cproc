struct {
	int : 4, x : 9, : 3;
} s;

int main(void) {
	s.x = 1;
	s.x += 3;
	return s.x == 4 ? 0 : 1;
}
