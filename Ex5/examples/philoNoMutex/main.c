#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>

struct philosopher_s;

typedef struct waiter_s
{
	int port[2];
} waiter_t;

typedef struct philosopher_s
{
	int port[2];
	enum { TALKING, WAITING, EATING } state;
	unsigned int dine;
	waiter_t* waiter;
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
	int forks[2];
	
	while (1)
	{
		// talk
		nsleep(roll(&seed, 1000000ul));
		
		// request forks
		write(self->waiter->port[1], &self, sizeof(self));
		
		// receive forks
		read(self->port[0], forks, sizeof(forks));
		
		// eat
		++self->dine;
		nsleep(roll(&seed, 1000000ul));
		
		// return forks
		write(self->waiter->port[1], &self, sizeof(self));
	}
	
	return NULL;
}

static inline unsigned neighbor(unsigned i, unsigned c, int d)
{
	return (i + c + d) % c;
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
	
	waiter_t waiter;
	philosopher_t philo[count];
	pthread_t threads[count];
	time_t start, now;
	
	// initialize waiter and philosophers
	pipe(waiter.port);
	for (int i = 0; i < count; ++i)
	{
		pipe(philo[i].port);
		philo[i].state = TALKING;
		philo[i].dine = 0;
		philo[i].waiter = &waiter;
	}
	
	// create the threads
	for (int i = 0; i < count; ++i)
		pthread_create(&threads[i], NULL, &philosopher, &philo[i]);
	
	// execute the waiter thread
	time(&start);
	while ((time(&now), difftime(now, start)) < duration)
	{
		philosopher_t* p;
		unsigned int i, o;
		int forks[2];
		
		// receive request
		read(waiter.port[0], &p, sizeof(p));
		i = p - philo;
		
		// what is the philosopher currently doing?
		switch (p->state)
		{
		case TALKING: // he wants to eat
			p->state = WAITING;
			if (philo[neighbor(i, count, -1)].state != EATING
			 && philo[neighbor(i, count, +1)].state != EATING)
			{
				forks[0] = i;
				forks[1] = (i + 1) % count;
				p->state = EATING;
				write(p->port[1], forks, sizeof(forks));
			}
			
			break;
			
		case EATING: // he is returning the forks
			p->state = TALKING;
			
			// check if his left neighbor wants to eat
			o = neighbor(i, count, -1);
			if (philo[o].state == WAITING
			 && philo[neighbor(o, count, -1)].state != EATING)
			{
				// pass on the forks
				forks[0] = o;
				forks[1] = i;
				philo[o].state = EATING;
				write(philo[o].port[1], forks, sizeof(forks));
			}
			
			// check if his right neighbor wants to eat
			o = neighbor(i, count, +1);
			if (philo[o].state == WAITING
			 && philo[neighbor(o, count, +1)].state != EATING)
			{
				// pass on the forks
				forks[0] = o;
				forks[1] = neighbor(o, count, +1);
				philo[o].state = EATING;
				write(philo[o].port[1], forks, sizeof(forks));
			}
			break;
			
		default:
			fprintf(stderr, "unaccounted state for philosopher %i\n", i);
			exit(-1);
			break;
		}
	}
	
	// cancel the threads and print the statistics
	for (int i = 0; i < count; ++i)
	{
		pthread_cancel(threads[i]);
		pthread_join(threads[i], NULL);
		close(philo[i].port[0]);
		close(philo[i].port[1]);
		
		printf("Philosopher %i ate %u times\n",
		       i, philo[i].dine);
	}
	
	close(waiter.port[0]);
	close(waiter.port[1]);
	return 0;
}
