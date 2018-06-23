#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>

pthread_mutex_t room;
unsigned long day, lastDay;
unsigned int prisoners;
int light;

static void* counter(void* data)
{
	unsigned int count = 1;
	
	do
	{
		pthread_mutex_lock(&room);
		lastDay = ++day;
		
		if (light)
		{
			count++;
			light = 0;
		}
		
		pthread_mutex_unlock(&room);
		sched_yield();
	}
	while (count < prisoners);
	
	return NULL;
}

static void* prisoner(void* data)
{
	int counted = 0;
	
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
	
	while (1)
	{
		pthread_testcancel();
		pthread_mutex_lock(&room);
		++day;
		
		if (!light && !counted)
		{
			counted = 1;
			light = 1;
		}
		
		pthread_mutex_unlock(&room);
		sched_yield();
	}
	
	return NULL;
}

int main(int argc, const char* argv[])
{
	int count = 100;
	
	// allow overriding the defaults by the command line arguments
	switch (argc)
	{
	case 2:
		count = atoi(argv[1]);
		/* fall through */
	case 1:
		printf("Prisoners: %d\n", count);
		fflush(stdout);
		break;
		
	default:
		printf("Usage: %s [prisoners]\n", argv[0]);
		return -1;
	}
	
	pthread_t threads[count];
	pthread_mutex_init(&room, NULL);
	
	for (prisoners = 1; prisoners <= count; ++prisoners)
	{
		light = day = 0;
		
		pthread_mutex_lock(&room);
		pthread_create(&threads[0], NULL, &counter, NULL);
		for (int i = 1; i < prisoners; ++i)
			pthread_create(&threads[i], NULL, &prisoner, NULL);
		pthread_mutex_unlock(&room);
		
		pthread_join(threads[0], NULL);
		
		for (int i = 1; i < prisoners; ++i)
		{
			pthread_cancel(threads[i]);
			pthread_join(threads[i], NULL);
		}
		
		printf("PC: %3d\tDays: %8lu\n", prisoners, lastDay);
		fflush(stdout);
	}
	
	pthread_mutex_destroy(&room);
	return 0;
}
