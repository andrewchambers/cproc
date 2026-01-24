enum {A = 1} x;
unsigned x;
enum {B = -1} y;
int y;

int main(void) {
	return (x == 0 && y == 0) ? 0 : 1;
}
