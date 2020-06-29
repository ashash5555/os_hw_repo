#include<unistd.h>
#include<string.h>
#include <sys/mman.h>
#include <math.h>

//Includes for testing
#include <stdlib.h>
#include <iostream>
#include <assert.h>

#define MAX_SIZE 1e8
#define MIN_SIZE 128

///======================================Data Structures======================================///


struct MallocMetadata
{
    size_t size;
    bool is_free;
    size_t size_used;
    MallocMetadata* next;
    MallocMetadata* prev;
};


/// general data about allocations
struct AllocationsData
{
    size_t num_of_free_blocks;
    size_t num_of_free_bytes;
    size_t num_of_allocated_blocks;
    size_t num_of_allocated_bytes;
    size_t num_of_meta_data_bytes;
    size_t size_of_meta_data;
};
///===========================================================================================///

static void* _aux_smalloc(size_t);
///==========================================Globals==========================================///
// static void* heap = sbrk(0);
static AllocationsData data = {0, 0, 0, 0, 0, sizeof(MallocMetadata)};

static void* _get_dummy_sbrk_allocs() {
    MallocMetadata* dummy = (MallocMetadata*)_aux_smalloc(data.size_of_meta_data);
    dummy->size = 0;
    dummy->is_free = false;
    dummy->size_used = 0;
    dummy->next = nullptr;
    dummy->prev = nullptr;

    return dummy;
}

static void* heap_sbrk = _get_dummy_sbrk_allocs(); //Must only used once!

///=======================================Aux functions=======================================///
static void _mark_free(MallocMetadata* to_mark) {
    if (to_mark->is_free) return;
    to_mark->is_free = true;
    to_mark->size_used = 0;

}

static void _mark_used(MallocMetadata* to_mark, size_t used_size) {
    to_mark->is_free = false;
    to_mark->size_used = used_size;

}


static void _add_allocation(MallocMetadata* to_add, MallocMetadata* head) {
    MallocMetadata* ptr = head;
    MallocMetadata* prev_ptr = nullptr;
    // void* heap_end = sbrk(0);


    while (ptr) {
        if (to_add->size > ptr->size) {
            prev_ptr = ptr;
            ptr = ptr->next;
            continue;
        }
        else if (to_add->size < ptr->size) {
            to_add->next = ptr;
            to_add->prev = ptr->prev;
            prev_ptr->next = to_add;
            ptr->prev = to_add;
            break;
        }

        /// if sizes are equal, sort by addrs.
        else {
            if (ptr < to_add) {
                if(ptr->next) {
                    ptr->next->prev = to_add;
                }
                to_add->next = ptr->next;
                to_add->prev = ptr;
                ptr->next = to_add;
                break;
            }
            else if (ptr > to_add) {
                to_add->next = ptr;
                to_add->prev = ptr->prev;
                ptr->prev = to_add;
                prev_ptr->next = to_add;
                break;
            }
            else {
                break;
            }
        }
    }

    if (ptr == nullptr) {
        prev_ptr->next = to_add;
        to_add->next = ptr;
        to_add->prev = prev_ptr;
    }

}

/// malloc from part 1
static void* _aux_smalloc(size_t size) {
    /// not checkin size < 0 because size_t = unsigned int
    if (size == 0 || size > MAX_SIZE) return nullptr;

    /// get current addrs of brk.
    /// if allocation succeeded, we will return this pointer
    /// and we will have a free allocated block ahead of it.
    void* curr_brk = sbrk(0);

    /// try to allocate space on heap
    void* new_brk = sbrk(size);

    if (new_brk == (void*)(-1)) {
        return nullptr;
    }

    return curr_brk;
}

static void _unlink_on_sort(MallocMetadata* p_metadata) {
    p_metadata->prev->next = p_metadata->next;
    if(p_metadata->next) p_metadata->next->prev = p_metadata->prev;
    p_metadata->next = nullptr;
    p_metadata->prev = nullptr;
}
static MallocMetadata* _get_metadata(void* p) {
    MallocMetadata* p_metadata = (MallocMetadata*)(static_cast<char*>(p) - data.size_of_meta_data);
    return p_metadata;
}

static void* _get_data_after_metadata(MallocMetadata* metadata) {
    // unsigned char* byteptr = reinterpret_cast<unsigned char*>(metadata);
    MallocMetadata* p = (MallocMetadata*)(static_cast<char*>((void*)metadata) + data.size_of_meta_data);
    return (void*)p;
}

//Must come with Unlink
void _reinsert(MallocMetadata* p_metadata) {
    p_metadata->next = nullptr;
    p_metadata->prev = nullptr;
    _add_allocation(p_metadata, (MallocMetadata*)heap_sbrk);
}

 ///===========================================================================================///

size_t _num_free_blocks() {
    MallocMetadata* ptr = ((MallocMetadata*)heap_sbrk)->next;   /// skipping dummy
    data.num_of_free_blocks = 0;
    while (ptr) {
        if (ptr->is_free) {
            data.num_of_free_blocks++;
        }
        ptr = ptr->next;
    }
    /// storing in this var for debug
    return data.num_of_free_blocks;
}
size_t  _num_free_bytes() {
    MallocMetadata* ptr = ((MallocMetadata*)heap_sbrk)->next;   /// skipping dummy
    data.num_of_free_bytes = 0;
    while (ptr) {
        if (ptr->is_free) {
            data.num_of_free_bytes += ptr->size;
        }

        ptr = ptr->next;
    }
    /// storing in this var for debug
    return data.num_of_free_bytes;
}

size_t _num_allocated_blocks() {
    MallocMetadata* ptr = ((MallocMetadata*)heap_sbrk)->next;   /// skipping dummy
    data.num_of_allocated_blocks = 0;
    while (ptr) {
        data.num_of_allocated_blocks++;
        ptr = ptr->next;
    }
    /// storing in this var for debug
    return data.num_of_allocated_blocks;
}

size_t _num_allocated_bytes() {
    MallocMetadata* ptr = ((MallocMetadata*)heap_sbrk)->next;   /// skipping dummy
    data.num_of_allocated_bytes = 0;
    while (ptr) {
        data.num_of_allocated_bytes += ptr->size;
        ptr = ptr->next;
    }
    /// storing in this var for debug
    return data.num_of_allocated_bytes;
}

size_t _size_meta_data() {return data.size_of_meta_data;}

size_t _num_meta_data_bytes() {
    MallocMetadata* ptr = ((MallocMetadata*)heap_sbrk)->next;   /// skipping dummy
    data.num_of_meta_data_bytes = 0;
    while (ptr) {
        data.num_of_meta_data_bytes += data.size_of_meta_data;
        ptr = ptr->next;
    }
    /// storing in this var for debug
    return data.num_of_meta_data_bytes;
}


void* smalloc (size_t size) {
    if (size == 0 || size > MAX_SIZE) return nullptr;
    MallocMetadata* new_metadata = nullptr;
    void* new_data = nullptr;
    MallocMetadata* ptr = nullptr;
    MallocMetadata* last_block = nullptr;

    ptr = (MallocMetadata*)heap_sbrk; // Get list dummy

    // list is sorted and iterate through the sbrk list
    while (ptr) {

        /// Check if the block is big enough
        if (ptr->size >= size && ptr->is_free) {
            _mark_used(ptr, size);
            void* new_ptr = _get_data_after_metadata(ptr);
            return new_ptr;
        }
        else {
            last_block = ptr;
            ptr = ptr->next;
        }
    }

    //No spot to allocate in the list
    new_metadata = (MallocMetadata*)_aux_smalloc(data.size_of_meta_data);
    if(!new_metadata) return nullptr; //Error code in smalloc
    new_data = _aux_smalloc(size);
    if(!new_data) return nullptr;     //Error code in smalloc

    new_metadata->size = size;
    new_metadata->is_free = false;
    new_metadata->size_used = size;

    //List is sorted by the size on the block, not actual usage
    _add_allocation(new_metadata, (MallocMetadata*)heap_sbrk);

    return new_data;
}

void* scalloc(size_t num, size_t size) {
    /// not checkin size < 0 because size_t = unsigned int
    if (size == 0 || num*size > MAX_SIZE) return nullptr;

    size_t calloc_size = num * size;
    void* ptr = smalloc(calloc_size);
    if (ptr != nullptr) {
        memset(ptr, 0, calloc_size);
    } else {
        return nullptr;     //Error code as per instructions
    }

    return ptr;
}

void sfree(void* p) {
    if (p == nullptr) return;

    MallocMetadata* p_metadata = _get_metadata(p);
    if (p_metadata->is_free) return;

    _mark_free(p_metadata);

}

void* srealloc(void* oldp, size_t size) {
    if (size == 0 || size > MAX_SIZE) return nullptr;

    if (oldp == nullptr) return smalloc(size);
    MallocMetadata* oldp_metadata = _get_metadata(oldp);

    //Takes care of both cases if oldp is taken or not.
    //Will mark the new size of the block with size param
    if (oldp_metadata->size >= size) {
        if(!oldp_metadata->is_free) {
            return oldp;
        }else { //is free
            _mark_used(oldp_metadata, size);
            return oldp;   /// reusing block
        }
    }

    void* ptr = smalloc(size);
    MallocMetadata* new_metadata = _get_metadata(ptr);

    if (ptr != nullptr) {
        memmove(ptr, oldp, oldp_metadata->size);
        new_metadata->is_free = false;
        sfree(oldp);
    } else {    //sbrk failed
        return nullptr;
    }

    return ptr;
}


// #define META_SIZE         sizeof(MallocMetadata)
// int main(){

//     void* p1 = smalloc(1000);
//     void* p2 = smalloc(2000);
//     void* p3 = smalloc(3000);
//     void* p4 = smalloc(4000);
//     void* p5 = smalloc(5000);

//     sfree(p3);
//     p3 = smalloc(500);
//     sfree(p3);
//     sfree(p4);
//     sfree(p5);
//     //p1,p2 taken. p3,p4,p5 free

//     assert(_num_allocated_blocks() == 5);
//     assert(_num_allocated_bytes() == 15000);
//     assert(_num_free_bytes() == 12000);
//     assert(_num_free_blocks() == 3);
//     assert(_num_meta_data_bytes() == (5 * META_SIZE));

//     //p1 = [1000,0], p2 = [2000,0], p3 = [3000,3000], 
//     //p4 = [4000,3000], p5 = [5000,5000]


//     void* p6 = srealloc(p4, 6000);
//     assert(_num_allocated_blocks() == 6);
//     assert(_num_allocated_bytes() == 21000);
//     assert(_num_free_bytes() == 12000);
//     assert(_num_free_blocks() == 3);
//     assert(_num_meta_data_bytes() == (6 * META_SIZE));

//     p4 = srealloc(p4, 1000);
//     assert(_num_allocated_blocks() == 6);
//     assert(_num_allocated_bytes() == 21000);
//     assert(_num_free_bytes() == 8000);
//     assert(_num_free_blocks() == 2);
//     assert(_num_meta_data_bytes() == (6 * META_SIZE));

//     // pi = [size,free]
//     //p1 = [1000,0], p2 = [2000,0], p3 = [3000,3000], 
//     //p4 = [4000,3000], p5 = [5000,5000], p6 = [6000,0]
    
//     for(int i=0; i < 250; ++i) {
//         *((int*)p1 + i) = 1;
//     }
//     for(int i=0; i < 250; ++i) {
//         assert(*((int*)p1 + i) == 1);
//     }

//     //Take the "1"s from p1 to p5 and free p1
//     p5 = srealloc(p1, 2000);
//     for(int i=0; i < 250; ++i) {
//         assert(*((int*)p3 + i) == 1);
//     }
//     assert(_num_allocated_blocks() == 6);
//     assert(_num_allocated_bytes() == 21000);
//     assert(_num_free_bytes() == 6000);
//     assert(_num_free_blocks() == 2);
//     assert(_num_meta_data_bytes() == (6 * META_SIZE));

//     //p1 = [1000,1000], p2 = [2000,0], p3 = [3000,1000], 
//     //p4 = [4000,3000], p5 = [5000,5000], p6 = [6000,0]

//     //should take all 5000 of p5
//     void* temp = srealloc(nullptr, 2000);


//     assert(_num_allocated_blocks() == 6);
//     assert(_num_allocated_bytes() == 21000);
//     assert(_num_free_bytes() == 1000);
//     assert(_num_free_blocks() == 1);
//     assert(_num_meta_data_bytes() == (6 * META_SIZE));

//     //p1 = [1000,1000], p2 = [2000,0], p3 = [3000,1000], 
//     //p4 = [4000,3000], p5 = [5000,3000], p6 = [6000,0]

//     p1 = scalloc(500, 2);
//     for(int i=0; i < 250; ++i) {
//         assert(*((int*)p1 + i) == 0);
//     }
//     assert(_num_allocated_blocks() == 6);
//     assert(_num_allocated_bytes() == 21000);
//     assert(_num_free_bytes() == 0);
//     assert(_num_free_blocks() == 0);
//     assert(_num_meta_data_bytes() == (6 * META_SIZE));

//     //p1 = [1000,0], p2 = [2000,0], p3 = [3000,1000], 
//     //p4 = [4000,3000], p5 = [5000,3000], p6 = [6000,0]

//     void* p7 = scalloc(4, 250);
//     for(int i=0; i < 250; ++i) {
//         assert(*((int*)p7 + i) == 0);
//     }

//     assert(_num_allocated_blocks() == 7);
//     assert(_num_allocated_bytes() == 22000);
//     assert(_num_free_bytes() == 0);
//     assert(_num_free_blocks() == 0);
//     assert(_num_meta_data_bytes() == (7 * META_SIZE));

//     //p1 = [1000,0], p2 = [2000,0], p3 = [3000,1000], 
//     //p4 = [4000,3000], p5 = [5000,3000], p6 = [6000,0]
//     //p7 = [1000,0]

//     sfree(p2);
//     p2 = srealloc(p7, 2000);

//     assert(_num_allocated_blocks() == 7);
//     assert(_num_allocated_bytes() == 22000);
//     assert(_num_free_bytes() == 1000);
//     assert(_num_free_blocks() == 1);
//     assert(_num_meta_data_bytes() == (7 * META_SIZE));

//     //p1 = [1000,0], p2 = [2000,0], p3 = [3000,1000], 
//     //p4 = [4000,3000], p5 = [5000,3000], p6 = [6000,0]
//     //p7 = [1000,1000]

//     //allows to use more in p4. increase from 1000 to 3000
//     p4 =  srealloc(p4, 3000);

//     assert(_num_allocated_blocks() == 7);
//     assert(_num_allocated_bytes() == 22000);
//     assert(_num_free_bytes() == 1000);
//     assert(_num_free_blocks() == 1);
//     assert(_num_meta_data_bytes() == (7 * META_SIZE));

//     //p1 = [1000,0], p2 = [2000,0], p3 = [3000,1000], 
//     //p4 = [4000,1000], p5 = [5000,3000], p6 = [6000,0]
//     //p7 = [1000,1000]


//     //p4 uses 3000 out of 4000 so any amount higher needs to be moved
//     void* p8 = srealloc(p4, 8000); 

//     assert(_num_allocated_blocks() == 8);
//     assert(_num_allocated_bytes() == 30000);
//     assert(_num_free_bytes() == 5000);
//     assert(_num_free_blocks() == 2);
//     assert(_num_meta_data_bytes() == (8 * META_SIZE));

//     //p1 = [1000,0], p2 = [2000,0], p3 = [3000,1000], 
//     //p4 = [4000,4000], p5 = [5000,3000], p6 = [6000,0]
//     //p7 = [1000,1000], p8 = [8000,0]

//     //***********************Failure tests**************/
//     void* error = nullptr;
//     error = smalloc(MAX_SIZE + 1);
//     error = smalloc(0);
//     error = scalloc(MAX_SIZE, MAX_SIZE);
//     error = scalloc(0, 1000);
//     sfree(error);
//     error = srealloc(p1, MAX_SIZE + 1);
//     error = srealloc(p1, 0);
    
//     assert(_num_allocated_blocks() == 8);
//     assert(_num_allocated_bytes() == 30000);
//     assert(_num_free_bytes() == 5000);
//     assert(_num_free_blocks() == 2);
//     assert(_num_meta_data_bytes() == (8 * META_SIZE));

//     std::cout << "good job!" << std::endl;

//     return 0;

// }

