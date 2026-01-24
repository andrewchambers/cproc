struct {
	unsigned : 4, x : 15, : 13;
} s;

int main(void) {
	s.x = 0x7fff;
	return s.x == 0x7fff ? 0 : 1;
}
