#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/time.h>

/** 
    Part 1: We noticed that the entries are getting lost during insertion due to race conditions. 
            We solved this by adding a mutex lock in order to restrict only one thread to insert at a time.

    Part 2: Changed mutex to spin lock in the parallel_spin.c source file.
            With Single Mutex: 1 thread: 0.013696s | 4 thread: 0.021591s | 6 thread: 0.109853s | 8 thread: 0.049921s | 12 thread: 0.027896s
            With Spin: 1 thread: 0.018088s | 4 thread: 0.041793s | 6 thread: 0.024035s | 8 thread: 0.041734s | 12 thread: 0.131861s
            This shows that using spin lock is slower than the use of mutex with multiple threads in our case. 
            This is because mutex yields the CPU by going to sleep if lock is already acquired whereas spin lock does busy waiting and hence hogs up the CPU cycles instead of yielding.
            This especially becomes a problem when the number of threads increases and most of them are busy waiting.

    Part 3: No changes required here as the lock is only required for insertion which was running into race condition. 
            Retrieval is more or less running in a single instruction and does not have the same issue.

    Part 4: Updated the code to add an array of locks, one for each bucket. 
            This way a thread is blocked only when another thread is trying to insert in that bucket otherwise 
            insertion can run in parallel.
*/

#define NUM_BUCKETS 5     // Buckets in hash table
#define NUM_KEYS 100000   // Number of keys inserted per thread
int num_threads = 1;      // Number of threads (configurable)
int keys[NUM_KEYS];
pthread_spinlock_t spinlock; // declaring a spin lock

typedef struct _bucket_entry {
    int key;
    int val;
    struct _bucket_entry *next;
} bucket_entry;

bucket_entry *table[NUM_BUCKETS];

void panic(char *msg) {
    printf("%s\n", msg);
    exit(1);
}

double now() {
    struct timeval tv;
    gettimeofday(&tv, 0);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

// Inserts a key-value pair into the table
void insert(int key, int val) {
    int i = key % NUM_BUCKETS;
    bucket_entry *e = (bucket_entry *) malloc(sizeof(bucket_entry));
    if (!e) panic("No memory to allocate bucket!");
    // acquire or wait on the spin lock before inserting the value
    pthread_spin_lock(&spinlock);
    e->next = table[i];
    e->key = key;
    e->val = val;
    table[i] = e;
    // releasing the lock before exiting the process
    pthread_spin_unlock(&spinlock);
}

// Retrieves an entry from the hash table by key
// Returns NULL if the key isn't found in the table
bucket_entry * retrieve(int key) {
    bucket_entry *b;
    for (b = table[key % NUM_BUCKETS]; b != NULL; b = b->next) {
        if (b->key == key) return b;
    }
    return NULL;
}

void * put_phase(void *arg) {
    long tid = (long) arg;
    int key = 0;

    // If there are k threads, thread i inserts
    //      (i, i), (i+k, i), (i+k*2)
    for (key = tid ; key < NUM_KEYS; key += num_threads) {
        insert(keys[key], tid);
    }

    pthread_exit(NULL);
}

void * get_phase(void *arg) {
    long tid = (long) arg;
    int key = 0;
    long lost = 0;

    for (key = tid ; key < NUM_KEYS; key += num_threads) {
        if (retrieve(keys[key]) == NULL) lost++;
    }
    printf("[thread %ld] %ld keys lost!\n", tid, lost);

    pthread_exit((void *)lost);
}

int main(int argc, char **argv) {
    long i;
    pthread_t *threads;
    double start, end;

    if (argc != 2) {
        panic("usage: ./parallel_hashtable <num_threads>");
    }
    if ((num_threads = atoi(argv[1])) <= 0) {
        panic("must enter a valid number of threads to run");
    }

    srandom(time(NULL));
    for (i = 0; i < NUM_KEYS; i++)
        keys[i] = random();

    threads = (pthread_t *) malloc(sizeof(pthread_t)*num_threads);
    if (!threads) {
        panic("out of memory allocating thread handles");
    }

    // initialize the spin lock
    pthread_spin_init(&spinlock, 0);

    // Insert keys in parallel
    start = now();
    for (i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, put_phase, (void *)i);
    }
    
    // Barrier
    for (i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    end = now();
    
    printf("[main] Inserted %d keys in %f seconds\n", NUM_KEYS, end - start);
    
    // Reset the thread array
    memset(threads, 0, sizeof(pthread_t)*num_threads);

    // Retrieve keys in parallel
    start = now();
    for (i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, get_phase, (void *)i);
    }

    // Collect count of lost keys
    long total_lost = 0;
    long *lost_keys = (long *) malloc(sizeof(long) * num_threads);
    for (i = 0; i < num_threads; i++) {
        pthread_join(threads[i], (void **)&lost_keys[i]);
        total_lost += lost_keys[i];
    }
    end = now();

    printf("[main] Retrieved %ld/%d keys in %f seconds\n", NUM_KEYS - total_lost, NUM_KEYS, end - start);

    return 0;
}