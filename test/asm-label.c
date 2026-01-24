int x __asm__("y");
int x = 2;

void f(void) __asm__("g");
void f(void) {}

int main(void) {
	f();
	return x == 2 ? 0 : 1;
}
