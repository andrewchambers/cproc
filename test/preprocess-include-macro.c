#define HDR "preprocess-include-macro.h"
#include HDR

int main(void) {
	return inc_value() == 37 ? 0 : 1;
}
