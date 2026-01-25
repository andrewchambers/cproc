#include "preprocess-pragma-once.h"
#include "preprocess-pragma-once.h"

int main(void) {
	struct pp_once v;
	v.a = PP_ONCE_VALUE;
	return v.a == 5 ? 0 : 1;
}
