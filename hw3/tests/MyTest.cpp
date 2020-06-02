#include "Barrier.h"
#include <iostream>
#include <pthread.h>
#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>

#define NUM_OF_THREADS 4
#define NUM_OF_LOOPS 2

static int count1 = 0;
static int error_loop = -1;
pthread_mutex_t count_lock = PTHREAD_MUTEX_INITIALIZER;

void* ThreadLooper(void* arg) {
	Barrier* p_barrier = (Barrier*)(arg);
	
	for(int i = 1; i <= NUM_OF_LOOPS; i++) {
		pthread_mutex_lock(&count_lock);
		count1++;
		pthread_mutex_unlock(&count_lock);
		p_barrier->wait(); //Call 1 to barrier
		if((error_loop >= 0) || (count1 != (NUM_OF_THREADS * i))) {
			error_loop = i;
			break;
		}
		p_barrier->wait(); //Call 2 to barrier
	}
	return NULL;
}

int main() {
	printf("Starting test...\n");
	Barrier barrier(NUM_OF_THREADS);
	pthread_t threads[NUM_OF_THREADS];

	//Loop to create all threads.
	for (int i = 0; i < NUM_OF_THREADS; ++i) {
		if (pthread_create(&threads[i], NULL, ThreadLooper, &barrier)) {
	 		printf("Failed create...\n");
    	}
    }
    /* Waiting for threads to finish... */
    //Loop to join back all threads.
    for (int i = 0; i < NUM_OF_THREADS; ++i) {
    	if (pthread_join(threads[i], NULL)) {
			printf("Join failed...\n");
    	}
	}
	if(error_loop == -1)
		printf("Finished, passed.\n");
	else
		printf("Finished, error at loop: %d\n", error_loop);
}
