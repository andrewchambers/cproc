int f(int v) {
	switch (v) {
	case 3: return 30;
	case 52: return 520;
	case -3: return -30;
	default: return 1;
	case 0: return 0;
	case 101: return 1010;
	}
}

int main(void) {
	return (f(3) == 30 &&
		f(52) == 520 &&
		f(-3) == -30 &&
		f(0) == 0 &&
		f(101) == 1010 &&
		f(7) == 1) ? 0 : 1;
}
