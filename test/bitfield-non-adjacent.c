struct {
	int x : 1, y, z : 1;
} s;

int main(void) {
	s.x = 1;
	s.y = 5;
	return s.y == 5 ? 0 : 1;
}
