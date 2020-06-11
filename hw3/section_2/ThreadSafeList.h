#ifndef THREAD_SAFE_LIST_H_
#define THREAD_SAFE_LIST_H_

#include <pthread.h>
#include <iostream>
#include <iomanip> // std::setw

using namespace std;

template <typename T>
class List {
public:
    /**
     * Constructor
     */
    List() : head(new DNode()), tail(new DNode()), mutex(PTHREAD_MUTEX_INITIALIZER), size(0) {
        head->next = tail;
        tail->next = nullptr;
    }

    /**
     * Destructor
     */
    ~List() {
        Node* it = head;
        Node* aux = nullptr;
        while (it) {
            aux = it->next;
            delete it;
            it = aux;
        }
        pthread_mutex_destroy(&mutex);
    }

    class Node {
    public:
        T data;
        pthread_mutex_t mutex;
        Node *next;
        // TODO: Add your methods and data members
        Node(const T& data) : data(data), mutex(PTHREAD_MUTEX_INITIALIZER), next(nullptr) {}
        Node(const Node&) = delete;
        virtual ~Node() {
//            delete data;
            pthread_mutex_destroy(&mutex);
        }

        void lock() {
            pthread_mutex_lock(&mutex);
        }
        void unlock() {
            pthread_mutex_unlock(&mutex);
        }

    };

    class DNode : public Node {
    public:
        DNode() : Node(0) {}
        ~DNode() override = default;
    };

    /**
     * Insert new node to list while keeping the list ordered in an ascending order
     * If there is already a node has the same data as @param data then return false (without adding it again)
     * @param data the new data to be added to the list
     * @return true if a new node was added and false otherwise
     */
    bool insert(const T& data) {
        Node* to_add = new Node(data);
        Node* it = head;
        Node* prev = nullptr;

        head->lock();
        head->next->lock();
        prev = head;
        it = prev->next;

        while (it && typeid(*it) != typeid(DNode)) {
            if (data > it->data) {
                it->next->lock();   /// now holding 3 locks     prev, it, it->next
                prev->unlock();     /// now holding 2 locks     it, it->next
                prev = it;  /// locked
                it = it->next;  /// locked

            }
            else if (data < it->data) {
                to_add->next = it;
                prev->next = to_add;
                lock();
                size++;
                unlock();
                break;
            }
            // equal
            else {
                delete to_add;
                to_add = nullptr;
                __insert_test_hook();
                it->unlock();
                prev->unlock();
                return false;
            }
        }

        if (typeid(*it) == typeid(DNode)) {
            prev->next = to_add;
            to_add->next = it;
            lock();
            size++;
            unlock();
        }

        __insert_test_hook();
        it->unlock();
        prev->unlock();

        return true;
    }

    /**
     * Remove the node that its data equals to @param value
     * @param value the data to lookup a node that has the same data to be removed
     * @return true if a matched node was found and removed and false otherwise
     */
    bool remove(const T& value) {
        Node* it = head;
        Node* prev = nullptr;

        head->lock();
        head->next->lock();
        prev = head;
        it = prev->next;

        while (it && typeid(*it) != typeid(DNode)) {
            if (it->data == value) {
                it->next->lock();
                prev->next = it->next;
                it->next->unlock();
                it->next = nullptr;

                __remove_test_hook();

                it->unlock();
                delete it;
                it = nullptr;
                lock();
                size--;
                unlock();
                prev->unlock();

                return true;
            }
            else if (value < it->data) {
                break;
            }
            else {

                it->next->lock();   /// now holding 3 locks     prev, it, it->next
                prev->unlock();     /// now holding 2 locks     it, it->next
                prev = it;  /// locked
                it = it->next;  /// locked

            }
        }

        __remove_test_hook();
        it->unlock();
        prev->unlock();

        return false;
    }


    /**
     * Returns the current size of the list
     * @return current size of the list
     */
    unsigned int getSize() {
        return size;
    }

    // Don't remove
    void print() {
        Node* temp = head;
        if (temp == NULL)
        {
            cout << "";
        }
        else if (temp->next == NULL)
        {
            cout << temp->data;
        }
        else
        {
            while (temp != NULL)
            {
                if (temp == head || temp == tail) {
                    temp = temp->next;
                    continue;
                }
                cout << right << setw(3) << temp->data;
                temp = temp->next;
                cout << " ";
            }
        }
        cout << endl;
    }

    // Don't remove
    virtual void __insert_test_hook() {}
    // Don't remove
    virtual void __remove_test_hook() {}

    void lock() {
        pthread_mutex_lock(&mutex);
    }
    void unlock() {
        pthread_mutex_unlock(&mutex);
    }

    // bool isSorted(){

    //     head->lock();
    //     head->next->lock();
    //     Node* prev = head;
    //     Node* curr = head->next;

    //     while(curr && typeid(*curr) != typeid(DNode)) {
    //         if(prev->data >= curr->data) {
    //             curr->unlock();
    //             prev->unlock();
    //             return false;
    //         }

    //         curr->next->lock();   /// now holding 3 locks     prev, it, it->next
    //         prev->unlock();     /// now holding 2 locks     it, it->next
    //         prev = curr;  /// locked
    //         curr = curr->next;  /// locked
    //     }

    //     curr->unlock();
    //     prev->unlock();
    //     return true;
    // }


private:
    DNode* head;
    DNode* tail;
    pthread_mutex_t mutex;
    unsigned int size;
// TODO: Add your own methods and data members
};

#endif //THREAD_SAFE_LIST_H_