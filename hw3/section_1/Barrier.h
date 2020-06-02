#ifndef BARRIER_H_
#define BARRIER_H_

#include <semaphore.h>

class Barrier {
private:
    sem_t sem;
    sem_t sem2;
    sem_t mutex;
    
    unsigned int num_of_threads;
    unsigned int counter;
public:
    Barrier(unsigned int num_of_threads);
    void wait();
    ~Barrier() = default;

    // aux function for tests...
    unsigned int waitingThreads();
	// TODO: define the member variables
	// Remember: you can only use semaphores!
};

#endif // BARRIER_H_

