#include <string.h>

int
main(void)
{
	const char *name = "foo.c";
	const char *slash, *dot;
	size_t baselen;

	slash = strrchr(name, '/');
	if (slash)
		name = slash + 1;
	dot = strrchr(name, '.');
	baselen = dot ? (size_t)(--dot - name + 1) : strlen(name);
	return baselen == 3 ? 0 : 1;
}
