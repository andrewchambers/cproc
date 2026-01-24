struct pair { int a; int b; };

int main(void) {
	volatile struct pair p;
	p.a = 3;
	p.b = 4;
	return (p.a == 3 && p.b == 4) ? 0 : 1;
}
