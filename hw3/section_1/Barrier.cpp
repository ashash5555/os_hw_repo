#include "Barrier.h"

Barrier::Barrier(unsigned int num_of_threads) : num_of_threads(num_of_threads), counter(0) {
    // if semaphore is initialized with 0, it will hold everyone from going in
    sem_init(&sem, 0, 0);
    sem_init(&sem2, 0, 0);

    // acts as a mutex
    sem_init(&mutex, 0, 1);
}

void Barrier::wait() {
    // locking before critical section
    sem_wait(&mutex);

    // count how many threads are waiting to pass
    counter++;
    
    // if we reached the maximum number of threads, start releasing signals
    if (counter == num_of_threads) {
        for (unsigned int i = 0; i < num_of_threads; i++) {
            sem_post(&sem);
        }
    }

    // unlock
    sem_post(&mutex);
    
    // this semaphore now have 'num_of_threads' threads passing
    // so they lock this section and start decreasing the counter
    sem_wait(&sem);
    sem_wait(&mutex);
    counter--;
    
    // if all of the threads got through, let them pass the second semaphore
    if (counter == 0) {
        for (unsigned int i = 0; i < num_of_threads; i++) {
            sem_post(&sem2);
        }
    }

    // unlock and salamat
    sem_post(&mutex);
    sem_wait(&sem2);
}


// aux function for tests...
unsigned int Barrier::waitingThreads() {
    return counter;
}