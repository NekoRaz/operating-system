#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>
#include <time.h>

int hasEnoughFunds(void* data);
int pay(void* data);

typedef struct collector_s{
    //    pthread_mutex_t* fork[2];
    unsigned int funds;
    unsigned int inboundCollections;
    unsigned int outboundCollections;
}collector_t;

static void* collector(void* data){
    collector_t* self = data;
    
    while (1) {
        // no cancelling in the critical section
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
        
        ++self->inboundCollections;
        if(hasEnoughFunds(self) == 1){
            int amount = pay(self);
            printf("amount: %i\n", amount);
            printf("funds: %i\n", self->funds);
        }
        
        // cancelling allowed
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
        pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
        pthread_testcancel();
        sched_yield();
    }
    
    return NULL;
}

int hasEnoughFunds(void* data){
    collector_t* self = data;
    int enough;
    if (self->funds>100) {
        enough = 1;
    }else{
        enough = 0;
    }
    return enough;
}

int pay(void* data){
    collector_t* self = data;
    int amount = 0;
    if (self->funds/2 < 100) {
        amount = 100;
    }else{
        amount = self->funds/2;
    }
    self->funds = self->funds-amount;
    
    return amount;
}

int main(int argc, const char* argv[])
{
    double duration = 2; // default duration in seconds
    int collectors = 5;  // default number of tax collectors
    int funds = 300;     // default funding per collector in Euro
    
    // allow overriding the defaults by the command line arguments
    switch (argc)
    {
        case 4:
            duration = atof(argv[3]);
            /* fall through */
        case 3:
            funds = atoi(argv[2]);
            /* fall through */
        case 2:
            collectors = atoi(argv[1]);
            /* fall through */
        case 1:
            printf(
                   "Tax Collectors:  %d\n"
                   "Initial funding: %d EUR\n"
                   "Duration:        %g s\n",
                   collectors, funds, duration
                   );
            break;
            
        default:
            printf("Usage: %s [collectors [funds [duration]]]\n", argv[0]);
            return -1;
    }
    
    collector_t coll[collectors];
    pthread_t threads[collectors];
    //    pthread_mutex_t forks[collectors];
    
    for (int i = 0; i<collectors; i++) {
        coll[i].funds = funds;
        coll[i].inboundCollections = 0;
        coll[i].outboundCollections = 0;
    }
    
    for (int i = 0; i<collectors; i++) {
        pthread_create(&threads[i], NULL, &collector, &coll[i]);
    }
    
    sleep(duration);
    
    int totalInboundCollections = 0;
    int totalOutboundCollections = 0;
    int totalMoney = 0;
    int expectedMoney = 0;
    
    for (int i = 0; i < collectors; ++i)
    {
        pthread_cancel(threads[i]);
        pthread_join(threads[i], NULL);
        
        expectedMoney = expectedMoney + funds;
        totalInboundCollections = totalInboundCollections + coll[i].inboundCollections;
        totalOutboundCollections = totalOutboundCollections + coll[i].outboundCollections;
        totalMoney = totalMoney + coll[i].funds;
        
        printf("Collector %i has %u funds, collected %i times and paid %i times\n",
               i, coll[i].funds, coll[i].inboundCollections, coll[i].outboundCollections);
        fflush(stdout);
    }
    
    printf("total number of collections: %i\n", totalInboundCollections);
    printf("total number of payments: %i\n", totalOutboundCollections);
    printf("amount of money in system: %i, expected: %i\n", totalMoney, expectedMoney);
    
    return 0;
}


