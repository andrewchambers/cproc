#include <pthread.h>
#include <stdint.h>

_Thread_local int arr[3];
_Thread_local int arr2[2] = {1, 2};

static void *thread_main(void *unused)
{
	(void)unused;
	if (arr[0] != 0 || arr[1] != 0 || arr2[0] != 1 || arr2[1] != 2)
		return (void *)(uintptr_t)1;
	arr[0] = 5;
	arr2[1] = 7;
	if (arr[0] != 5 || arr2[1] != 7)
		return (void *)(uintptr_t)2;
	return NULL;
}

int main(void)
{
	pthread_t thr;
	void *ret = NULL;

	if (arr[0] != 0 || arr[1] != 0 || arr2[0] != 1 || arr2[1] != 2)
		return 1;
	if (pthread_create(&thr, NULL, thread_main, NULL) != 0)
		return 2;
	if (pthread_join(thr, &ret) != 0)
		return 3;
	if (ret)
		return 4;
	if (arr[0] != 0 || arr2[1] != 2)
		return 5;
	return 0;
}
