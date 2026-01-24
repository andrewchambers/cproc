struct {
	signed : 4, x : 15, : 13;
} s;

int main(void) {
	s.x = -1;
	return s.x == -1 ? 0 : 1;
}
