#ifndef THREAD_SAFE_LIST_H_
#define THREAD_SAFE_LIST_H_

#include <pthread.h>
#include <iostream>
#include <stdio.h>
#include <iomanip> // std::setw

using namespace std;

template <typename T>
class List 
{
public:
    /**
     * Constructor
     */
    List() : head(nullptr), size(0), size_lock(PTHREAD_MUTEX_INITIALIZER), head_lock(PTHREAD_MUTEX_INITIALIZER) { //TODO: add your implementation


//	        std::cerr << “<function name>: failed” << endl;
//          exit(-1);
    }

    /**
     * Destructor
     */
    virtual ~List(){ //TODO: add your implementation
        delete head;
	    pthread_mutex_destroy(&size_lock);
        pthread_mutex_destroy(&head_lock);
    }

    class Node {
     public:
      T* data;
      Node *next;
      pthread_mutex_t m;
      // TODO: Add your methods and data members
      explicit Node(const T& data) : data(new T(data)), next(nullptr), m(PTHREAD_MUTEX_INITIALIZER) {}

      ~Node() {
      	delete data;
      	delete next;
      	pthread_mutex_destroy(&m);
      }

      bool insert(const T& toInsert, pthread_mutex_t* prev_lock) {
      	//locks to "this" & prev node already locked by prev
      	//new data already exists - return false
      	if (*data == toInsert) {
	        pthread_mutex_unlock(&m);
      	    pthread_mutex_unlock(prev_lock);
      	}
      	//new data is bigger, therefore needs to appear after "this"
        else if (*data < toInsert) {
      		if (next) {
		        pthread_mutex_lock(&(next->m));
		        //new data should be inserted before the current "next"
		        if (*(next->data) > toInsert) {
		        	Node *temp = new Node(toInsert);
			        temp->next = next;
			        next = temp;
			        pthread_mutex_unlock(&(temp->next->m));
			        pthread_mutex_unlock(&m);
			        pthread_mutex_unlock(prev_lock);
			        return true;
		        }
		        //new data should be inserted after the current "next"
		        else {
			        pthread_mutex_unlock(prev_lock);
		        	return next->insert(toInsert, &m);
		        }
	        }
      		//there is no next - insert here
      		else {
		        Node *temp = new Node(toInsert);
		        next = temp;
		        pthread_mutex_unlock(&m);
		        pthread_mutex_unlock(prev_lock);
		        return true;
	        }
      	}
        return false;
      }

	    bool remove(const T& value, pthread_mutex_t* prev_lock) {
		    //locks to "this" & prev node already locked by prev
      	    //this->data is bigger than value, therefore all other elements in the list will also be greater
      	    if (*data > value) {
      	    	pthread_mutex_unlock(&m);
		        pthread_mutex_unlock(prev_lock);
      	    	return false;
      	    }
      	    //this->data is smaller than value, therefore it might appear later in the list
      	    else if (*data < value) {

      	    	//there is no tail to the list, value isn't in it
      	    	if (!next) {
		            pthread_mutex_unlock(&m);
		            pthread_mutex_unlock(prev_lock);
		            return false;
	            }

      	    	//there is a next node, get the lock to work with it
      	    	pthread_mutex_lock(&(next->m));

      	    	//next->data is what needs to be removed, remove it
      	    	if (*(next->data) == value) {
		            if(next->next) pthread_mutex_lock(&(next->next->m));
      	    		Node* temp = next;
      	    		next = temp->next;
      	    		temp->next = nullptr;
		            pthread_mutex_unlock(&(temp->m));
					delete temp;
		            if(next) pthread_mutex_unlock(&(next->m));
      	            pthread_mutex_unlock(&m);
		            pthread_mutex_unlock(prev_lock);
	            } else {
		            pthread_mutex_unlock(prev_lock);
      	    		return next->remove(value, &m);
      	    	}
      	    }
      	    return true;
        }
    };

    /**
     * Insert new node to list while keeping the list ordered in an ascending order
     * If there is already a node has the same data as @param data then return false (without adding it again)
     * @param data the new data to be added to the list
     * @return true if a new node was added and false otherwise
     */
    bool insert(const T& data) {
		//TODO: add your implementation
	    bool res = true;
		pthread_mutex_lock(&head_lock);
		if (head) {
			pthread_mutex_lock(&(head->m));
			//new data is smaller than the head of the list, make a new head
			if (*(head->data) > data) {
				Node *temp = new Node(data);
				temp->next = head;
				head = temp;
				__add_hook();
				pthread_mutex_unlock(&(temp->next->m));
				pthread_mutex_unlock(&head_lock);
			}
			//try to insert the data into the list
			else {
				Node* temp = head;
				res = temp->insert(data, &head_lock);
				if(res) __add_hook();
			}
		}
		//there is no head - create a new one
		else {
			head = new Node(data);
			__add_hook();
			pthread_mutex_unlock(&head_lock);
		}
	    if (res) {
		    pthread_mutex_lock(&size_lock);
		    ++size;
		    pthread_mutex_unlock(&size_lock);
	    }
		return res;
    }

    /**
     * Remove the node that its data equals to @param value
     * @param value the data to lookup a node that has the same data to be removed
     * @return true if a matched node was found and removed and false otherwise
     */
    bool remove(const T& value) {
		//TODO: add your implementation
	    bool res = true;
		pthread_mutex_lock(&head_lock);
	    //the list is empty
	    if (!head) {
		    pthread_mutex_unlock(&head_lock);
		    return false;
	    }
	    //there is at least 1 element in the list
	    else {
	    	pthread_mutex_lock(&(head->m));
	    	//the head should be removed
	    	if (*(head->data) == value) {
			    if(head->next) pthread_mutex_lock(&(head->next->m));
	            Node* temp = head;
	            head = temp->next;
	            temp->next = nullptr;
	            pthread_mutex_unlock(&(temp->m));
	            delete temp;
	            __remove_hook();
			    if(head) pthread_mutex_unlock(&(head->m));
	            pthread_mutex_unlock(&head_lock);
	    	}
	        //try to remove value from the list
	    	else {
	    		res = head->remove(value, &head_lock);
	    		if (res) __remove_hook();
	    	}
	    }
	    if (res) {
		    pthread_mutex_lock(&size_lock);
		    --size;
		    pthread_mutex_unlock(&size_lock);
	    }
	    return res;
    }

    /**
     * Returns the current size of the list
     * @return the list size
     */
    unsigned int getSize() {
		//TODO: add your implementation
		return size;
    }

	// Don't remove
    void print() {
      pthread_mutex_lock(&head_lock);
      Node* temp = head;
      if (temp == NULL)
      {
        cout << "";
      }
      else if (temp->next == NULL)
      {
        cout << *(temp->data);
      }
      else
      {
        while (temp != NULL)
        {
          cout << right << setw(3) << *(temp->data);
          temp = temp->next;
          cout << " ";
        }
      }
      cout << endl;
      pthread_mutex_unlock(&head_lock);
    }

	// Don't remove
    virtual void __add_hook() {}
	// Don't remove
    virtual void __remove_hook() {}

	bool isSorted(){
		pthread_mutex_lock(&head_lock);
		if(!head) {
			pthread_mutex_unlock(&head_lock);
			return true;
		}else{
			pthread_mutex_lock(&head->m);
		}
		Node* prev = head;
		Node* curr = head->next;
		while(curr) {
			pthread_mutex_lock(&curr->m);
			if(*prev->data >= *curr->data) {
				pthread_mutex_unlock(&curr->m);
				pthread_mutex_unlock(&prev->m);
				return false;
			}
			pthread_mutex_unlock(&prev->m);
			prev = curr;
			curr = curr->next;
		}
		pthread_mutex_unlock(&prev->m);
		pthread_mutex_unlock(&head_lock);
		return true;
	}

private:
    Node* head;
    // TODO: Add your own methods and data members
	unsigned int size;
	pthread_mutex_t size_lock;
	pthread_mutex_t head_lock;
};

#endif //THREAD_SAFE_LIST_H_