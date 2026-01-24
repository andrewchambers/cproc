unsigned char u8 = u8'a';
static_assert(__builtin_types_compatible_p(typeof(u8'b'), unsigned char),
	"UTF-8 character constant has incorrect type");

int main(void) {
	return u8 == (unsigned char)'a' ? 0 : 1;
}
