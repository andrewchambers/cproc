struct s {
	int x;
};

struct s f(void) {
	return (struct s){2};
}

int main(void) {
	return f().x == 2 ? 0 : 1;
}
