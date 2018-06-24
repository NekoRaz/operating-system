#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>
#include <time.h>

int hasEnoughFunds(void* data);
int pay(void* data);
int getRandomOtherCollector(int notThis);
void transaction(void* dataSelf,void* dataOther);

typedef struct collector_s{
    pthread_mutex_t wallet;
    unsigned int id;
    unsigned int funds;
    unsigned int inboundCollections;
    unsigned int outboundCollections;
}collector_t;

collector_t *coll;
int collectors;

static void* collector(void* data){
    collector_t* self = data;
    
    while (1) {
        // choose a random other collector to collect from
        int randomOtherCollector = getRandomOtherCollector(self->id);
        collector_t* other = &coll[randomOtherCollector];
        
        // no cancelling in the critical section
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
        
        // check if collector has enough money
        if(hasEnoughFunds(other) == 1){
            // if collector has enough money lock both collectors for transaction
            if (pthread_mutex_trylock(&self->wallet) == 0 && pthread_mutex_trylock(&other->wallet) == 0) {
                // do the transaction
                transaction(self,other);
                // unlock both collectors again
                pthread_mutex_unlock(&other->wallet);
                pthread_mutex_unlock(&self->wallet);
            }
        }else{
            // if collector has not enough money wait until he does
            while(hasEnoughFunds(other) == 0){
                pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
                pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
                pthread_testcancel();
            }
            // other collector has enough money again:
            // no cancelling in the critical section
            // lock both collectors for transaction
            pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
            if (pthread_mutex_trylock(&self->wallet) == 0 && pthread_mutex_trylock(&other->wallet) == 0) {
                transaction(self,other);
                // unlock both collectors again
                pthread_mutex_unlock(&other->wallet);
                pthread_mutex_unlock(&self->wallet);
            }
        }
        
        // cancelling allowed
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
        pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
        pthread_testcancel();
        // yield after successful transaction
        sched_yield();
    }
    
    return NULL;
}

// creates a random Collector ID which is not the Collectors own ID
int getRandomOtherCollector(int notThis){
    int ran = rand() % (collectors + 1);
    if(ran == notThis){
        ran = getRandomOtherCollector(notThis);
    }
    return ran;
}

// check if the given collector has enough funds
int hasEnoughFunds(void* data){
    collector_t* self = data;
    int enough;
    if (self->funds>=100) {
        enough = 1;
    }else{
        enough = 0;
    }
    return enough;
}

// transfers funds from one collector to another and increases their counters
void transaction(void* dataSelf,void* dataOther){
    collector_t* self = dataSelf;
    collector_t* other = dataOther;
    int amount = 0;
    
    if (other->funds/2 < 100) {
        amount = 100;
    }else{
        amount = other->funds/2;
    }
    // reduse funds of other collector
    other->funds = other->funds-amount;
    // increase pay counter of other collector
    ++other->outboundCollections;
    // increase collection counter of collecting collector
    ++self->inboundCollections;
    // increase funds of collecting collector
    self->funds = self->funds + amount;
}

int main(int argc, const char* argv[])
{
    double duration = 2; // default duration in seconds
    collectors = 5;  // default number of tax collectors
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
    
    //collector array has to be initialized
    coll = malloc(sizeof(collector_t) * collectors +1);
    pthread_t threads[collectors];
    
    //initialize collectors with given arguments
    for (int i = 0; i<collectors; i++) {
        coll[i].id = i;
        coll[i].funds = funds;
        coll[i].inboundCollections = 0;
        coll[i].outboundCollections = 0;
        if (pthread_mutex_init(&coll[i].wallet, NULL) != 0 )
            printf( "mutex init failed\n" );
        
    }
    
    // create threads and run szenario
    for (int i = 0; i<collectors; i++) {
        pthread_create(&threads[i], NULL, &collector, &coll[i]);
    }
    
    sleep(duration);
    
    int totalInboundCollections = 0;
    int totalOutboundCollections = 0;
    int totalMoney = 0;
    int expectedMoney = 0;
    
    // stop all threads and print statistics
    for (int i = 0; i < collectors; ++i)
    {
        pthread_cancel(threads[i]);
        pthread_join(threads[i], NULL);
        
        // count expected money
        expectedMoney = expectedMoney + funds;
        // count number of collections
        totalInboundCollections = totalInboundCollections + coll[i].inboundCollections;
        // count number of payments
        totalOutboundCollections = totalOutboundCollections + coll[i].outboundCollections;
        // count money of all collectors
        totalMoney = totalMoney + coll[i].funds;
        
        printf("Collector %i has %u funds, collected %i times and paid %i times\n",
               i, coll[i].funds, coll[i].inboundCollections, coll[i].outboundCollections);
        fflush(stdout);
    }
    
    printf("total number of collections: %i\n", totalInboundCollections);
    printf("total number of payments: %i\n", totalOutboundCollections);
    printf("amount of money in system: %i, expected: %i\n", totalMoney, expectedMoney);
    
    // destroy locks
    for (int i = 0; i < collectors; ++i)
        pthread_mutex_destroy(&coll[i].wallet);
    
    return 0;
}


