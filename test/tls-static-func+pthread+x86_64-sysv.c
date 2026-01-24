#include <pthread.h>
#include <stdint.h>

static _Thread_local int g = 1;

static int *tls_slot(void)
{
	static _Thread_local int x = 9;
	return &x;
}

static void *thread_main(void *unused)
{
	(void)unused;
	int *p = tls_slot();
	if (*p != 9)
		return (void *)(uintptr_t)1;
	*p = 11;
	g = 3;
	if (*p != 11 || g != 3)
		return (void *)(uintptr_t)2;
	return (void *)p;
}

int main(void)
{
	pthread_t thr;
	void *ret = NULL;
	int *p1 = tls_slot();
	int *p2 = tls_slot();

	if (p1 != p2)
		return 1;
	if (*p1 != 9)
		return 2;
	g = 2;

	if (pthread_create(&thr, NULL, thread_main, NULL) != 0)
		return 3;
	if (pthread_join(thr, &ret) != 0)
		return 4;
	if (ret == NULL)
		return 5;
	if (ret == (void *)p1)
		return 6;
	if (*p1 != 9)
		return 7;
	if (g != 2)
		return 8;
	return 0;
}
