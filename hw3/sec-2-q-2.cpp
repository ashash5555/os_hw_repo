typedef struct condvar {
    mutex m;
    singlephore h;
    unsigned int count;
    unsigned int value;
} condvar;
// Initilize the condition variable
void cond_init(condvar* c) {
    pthread_mutex_init(&(c->m));
    singlephore_init(&(c->h));
    count = 0;
    value = 0;
}
// Signal the condition variable
void cond_signal(condvar* c) {
    c->m->lock();
    if (count - value > 0) {
        H(&(c->h), MIN_INT, 1)
        value++;
    }
    c->m->unlock();
}
// Block until the condition variable is signaled. The mutex m must be locked by
the
// current thread. It is unlocked before the wait begins and re-locked after the
wait
// ends. There are no sleep-wakeup race conditions: if thread 1 has m locked and
// executes cond_wait(c,m), no other thread is waiting on c, and thread 2 executes
// mutex_lock(m); cond_signal(c); mutex_unlock(m), then thread 1 will always
recieve the
// signal (i.e., wake up).
void cond_wait(condvar* c, mutex* m) {
    c->m->lock();
    count++;
    c->m->unlock();
    m->unlock();
    H(&c->h, 1, -1);
    m->lock();
    c->m->lock();
    count--;
    value--;
    c->m->unlock();
}
