enum { line_a = __LINE__ };
enum { line_b = __LINE__ };

const char *file_name = __FILE__;

enum { counter_a = __COUNTER__ };
enum { counter_b = __COUNTER__ };

int main(void) {
	if (!(line_b == line_a + 1))
		return 1;
	if (!file_name || file_name[0] == 0)
		return 2;
	if (!(counter_b == counter_a + 1))
		return 3;
	if (sizeof(__DATE__) != 12)
		return 4;
	if (sizeof(__TIME__) != 9)
		return 5;
	return 0;
}
