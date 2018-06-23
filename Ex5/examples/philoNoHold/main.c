#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>

typedef struct philosopher_s
{
	pthread_mutex_t* fork[2];
	unsigned int dine;
} philosopher_t;

static unsigned int hash(const void* key, size_t size)
{
	const char* ptr = key;
	unsigned int hval;
	
	for (hval = 0x811c9dc5u; size --> 0; ++ptr)
	{
		hval ^= *ptr;
		hval *= 0x1000193u;
	}
	
	return hval;
}

static unsigned long roll(unsigned int* seed, unsigned long sides)
{
	return rand_r(seed) / (RAND_MAX + 1.0) * sides;
}

static void nsleep(unsigned long nano)
{
	struct timespec delay = {
		.tv_sec = 0,
		.tv_nsec = nano
	};
	nanosleep(&delay, NULL);
}

static void* philosopher(void* data)
{
	philosopher_t* self = data;
	time_t now = time(NULL);
	unsigned int seed = hash(&now, sizeof(now));
	
	while (1)
	{
		// talk for up to a tenth of a second
		nsleep(roll(&seed, 1000000ul));
		
		// no cancelling in the critical section
		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
		pthread_mutex_lock(self->fork[0]);
		
		while (!pthread_mutex_trylock(self->fork[1]))
		{
			pthread_mutex_unlock(self->fork[0]);
			pthread_mutex_lock(self->fork[0]);
		}
		
		++self->dine;
		
		// dine for up to a tenth of a second
		nsleep(roll(&seed, 1000000ul));
		
		pthread_mutex_unlock(self->fork[1]);
		pthread_mutex_unlock(self->fork[0]);
		
		// cancelling allowed
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	}
	
	return NULL;
}

int main(int argc, const char* argv[])
{
	int count = 5;
	int duration = 2;
	
	// allow overriding the defaults by the command line arguments
	switch (argc)
	{
	case 3:
		duration = atoi(argv[2]);
		/* fall through */
	case 2:
		count = atoi(argv[1]);
		/* fall through */
	case 1:
		printf("Philosophers: %d\n", count);
		fflush(stdout);
		break;
		
	default:
		printf("Usage: %s [philosophers [duration]]\n", argv[0]);
		return -1;
	}
	
	philosopher_t philo[count];
	pthread_t threads[count];
	pthread_mutex_t forks[count];
	
	for (int i = 0; i < count; ++i)
	{
		int left = i, right = (i + 1) % count;
		
		philo[i].dine = 0;
		philo[i].fork[0] = &forks[left];
		philo[i].fork[1] = &forks[right];
		
		pthread_mutex_init(&forks[i], NULL);
		pthread_mutex_lock(&forks[i]);
	}
	
	for (int i = 0; i < count; ++i)
		pthread_create(&threads[i], NULL, &philosopher, &philo[i]);
	
	for (int i = 0; i < count; ++i)
		pthread_mutex_unlock(&forks[i]);
	
	sleep(duration);
	
	for (int i = 0; i < count; ++i)
	{
		pthread_cancel(threads[i]);
		pthread_join(threads[i], NULL);
		
		printf("Philosopher %i ate %u times\n",
		       i, philo[i].dine);
		fflush(stdout);
	}
	
	for (int i = 0; i < count; ++i)
		pthread_mutex_destroy(&forks[i]);
	
	return 0;
}
