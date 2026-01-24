#include <pthread.h>
#include <stdint.h>

_Thread_local int v1;
_Thread_local int v2 = 5;
int g = 7;

static void *thread_main(void *unused)
{
	(void)unused;
	if (v1 != 0 || v2 != 5 || g != 7)
		return (void *)(uintptr_t)1;
	v1 = 1;
	v2 = 2;
	g = 3;
	if (v1 != 1 || v2 != 2 || g != 3)
		return (void *)(uintptr_t)2;
	return NULL;
}

int main(void)
{
	pthread_t thr;
	void *ret = NULL;

	if (v1 != 0 || v2 != 5 || g != 7)
		return 1;
	if (pthread_create(&thr, NULL, thread_main, NULL) != 0)
		return 2;
	if (pthread_join(thr, &ret) != 0)
		return 3;
	if (ret)
		return 4;
	if (v1 != 0 || v2 != 5 || g != 3)
		return 5;
	return 0;
}
