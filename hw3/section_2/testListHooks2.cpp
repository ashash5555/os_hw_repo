
#include "ThreadSafeList.h"
#include <iostream>
#include <stdio.h>
#include <assert.h>
#include "Barrier.h"
using namespace std;

#define N 610 //should be at least 6

Barrier b(N+1);

template <typename T>
class MyList : public List<T> {
public:
    void __insert_test_hook() override {
        if (!hook_enabled)
            return;
        this->remove(val_to_remove);
    }
    void activateHook(int val) {
        hook_enabled = true;
        val_to_remove = val;
    }
    void deactivateHook() {
        hook_enabled = false;
    }
    void setValueToRemove(int val) {
        val_to_remove = val;
    }
private:
    bool hook_enabled = false;
    int val_to_remove = 0;
};

class Job {
public:
    List<int>* l;
    int element;
    Job(List<int>* _l, int _element) : l(_l), element(_element) {}
};

void* removeElements(void* args) {
    Job* params = (Job*)args;
//	printf("about to remove %d...\n", params->element);
    params->l->remove(params->element);
//	printf("removed %d, waiting...\n", params->element);
    b.wait();
    return nullptr;
}


void* insertElements(void* args) {
    Job* params = (Job*)args;
//	printf("about to insert %d...\n", params->element);
    params->l->insert(params->element);
//	printf("inserted %d, waiting...\n", params->element);
//    return removeElements(args);
	b.wait();
	return nullptr;
}


int main() {
    List<int>* l = new MyList<int>();
    pthread_t t[N];
    Job* jobs[N];
    for (int i = 0; i < N; ++i) {
        jobs[i] = new Job(l, i);
        t[i] = pthread_create(t+i, NULL, insertElements, jobs[i]);
    }

//  l->insert(i);

    b.wait();

    l->print(); // should print: 0,2,4,6,8,10,12,14,16,18
    assert(l->getSize() == N);

  assert(!l->insert(0)); //make sure doubles can't be inserted (head)
  assert(!l->insert(2)); //make sure doubles can't be inserted (body)


    // the node to be removed should be a pred of the one being inserted
    // because hand locks are already acquired on the inserted node
    l->insert(N+20);

  ((MyList<int>*)l)->activateHook(N/3);
    l->insert(N+41);

    assert(l->getSize() == N+1);
    l->print(); // should print: 0,2,4,8,11,12,14,16,17,18
    for(int i = 0 ; i < N; ++i) {
        pthread_join(t[i], NULL);
        delete jobs[i];
    }
    return 0;
}
