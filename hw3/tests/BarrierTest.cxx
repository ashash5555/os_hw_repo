#include "Barrier.h"
#include <iostream>
#include <pthread.h>
#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>

#define NUM_OF_THREADS 10

typedef struct {
   Barrier* barrier;
   int num;
} ThreadHandlerArgs;

void* ThreadHandler(void* thread_handler_args);
   
int main() {
   // CREATE THREADS
   // EVERY TIME A THREAD ARRIVES TO A MOVIE PRINT IT
   // FINISH AND GO TO SLEEP
   Barrier barrier(NUM_OF_THREADS);
   // WAIT FOR THE THREADS TO FINISH
   pthread_t threads[NUM_OF_THREADS];
   for (int i = 0; i < NUM_OF_THREADS; ++i) {
      ThreadHandlerArgs *args = (ThreadHandlerArgs*)malloc(sizeof(*args));
      args->barrier = &barrier;
      args->num = i;
      if (pthread_create(&threads[i], NULL, ThreadHandler, args)) {
	 fprintf(stderr, "Failed...\n");
      }
   }
   for (int i = 0; i < NUM_OF_THREADS; ++i) {
      if (pthread_join(threads[i], NULL)) {
	 fprintf(stderr, "Failed...\n");
      }
   }
   
   std::cout << "Everyone have made it!" << std::endl;
   return 0;
}

void* ThreadHandler(void* thread_handler_args) {
   ThreadHandlerArgs* recv_args  = (ThreadHandlerArgs*)thread_handler_args;
   Barrier* bari_t = recv_args->barrier;
   int num = recv_args->num;
   bari_t->wait();
   std::cout << "Thread number " << num << " has arrived"  << std::endl;
   free(recv_args);
   return NULL;
}
