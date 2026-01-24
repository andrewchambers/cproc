typedef __typeof__(nullptr) nullptr_t;
static_assert(sizeof(nullptr_t) == sizeof(char *));

int main(void) {
	return ((void *)nullptr == 0) ? 0 : 1;
}
