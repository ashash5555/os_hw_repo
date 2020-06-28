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
#define MIN_MMAP_SIZE 128e3

///======================================Data Structures======================================///


struct MallocMetadata
{
    size_t size;
    bool is_free;
    bool is_mmapped;
    MallocMetadata* next;
    MallocMetadata* prev;
    MallocMetadata* next_on_heap;
    MallocMetadata* prev_on_heap;
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
    dummy->is_mmapped = false;
    dummy->next = nullptr;
    dummy->prev = nullptr;
    dummy->next_on_heap = nullptr;
    dummy->prev_on_heap = nullptr;

    return dummy;
}

static void* _get_dummy_mmap_allocs() {
    MallocMetadata* dummy = (MallocMetadata*)_aux_smalloc(data.size_of_meta_data);
    dummy->size = 0;
    dummy->is_free = false;
    dummy->is_mmapped = true;
    dummy->next = nullptr;
    dummy->prev = nullptr;

    return dummy;
}

static void* heap_sbrk = _get_dummy_sbrk_allocs();
static void* heap_mmap = _get_dummy_mmap_allocs();
//static MallocMetadata* tail = (MallocMetadata*)_get_dummy_sbrk_allocs();
static MallocMetadata* tail = (MallocMetadata*)heap_sbrk;
///=======================================Aux functions=======================================///
static void _mark_free(MallocMetadata* to_mark) {
    if (to_mark->is_free) return;
    to_mark->is_free = true;

    /// update data
//    data.num_of_free_blocks++;
//    data.num_of_free_bytes += to_mark->size;
}

static void _mark_used(MallocMetadata* to_mark) {
    to_mark->is_free = false;

    /// update data
//    data.num_of_free_blocks--;
//    data.num_of_free_bytes -= to_mark->size;
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

    /// update data
//    data.num_of_allocated_blocks++;
//    data.num_of_allocated_bytes += to_add->size;
//    data.num_of_meta_data_bytes += data.size_of_meta_data;
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

void _reinsert(MallocMetadata* p_metadata) {
    //p_metadata->prev->next = p_metadata->next;
    //if(p_metadata->next) p_metadata->next->prev = p_metadata->prev;
    p_metadata->next = nullptr;
    p_metadata->prev = nullptr;
    //data.num_of_allocated_bytes -= p_metadata->size;
    //data.num_of_meta_data_bytes -= data.size_of_meta_data;
    //data.num_of_allocated_blocks--;
    _add_allocation(p_metadata, (MallocMetadata*)heap_sbrk);
}

void _split(void* p, size_t size) {
    MallocMetadata* p_metadata = (MallocMetadata*)p;
    size_t new_block_size = p_metadata->size - size;
    size_t offset = data.size_of_meta_data + size;
    MallocMetadata* new_metadata = (MallocMetadata*)(static_cast<char*>(p) + offset);
    new_metadata->size = new_block_size - data.size_of_meta_data;
    new_metadata->is_free = true;
    new_metadata->is_mmapped = false;

    //Heap list link
    new_metadata->next_on_heap = p_metadata->next_on_heap;
    new_metadata->prev_on_heap = p_metadata;
    p_metadata->next_on_heap = new_metadata;
    if (new_metadata->next_on_heap) new_metadata->next_on_heap->prev_on_heap = new_metadata;    /// added this
    p_metadata->size = size;

    /// which head we need to send??? Roman: Only heap since mmap are deleted when freed so are never split
    ///
    _add_allocation(new_metadata, (MallocMetadata*)heap_sbrk);

//    data.num_of_allocated_bytes -= new_metadata->size + data.size_of_meta_data;
//    data.num_of_free_bytes -= data.size_of_meta_data;
//    data.num_of_free_blocks++;
    _unlink_on_sort(p_metadata);
    _reinsert(p_metadata);
}


bool _should_split(void* p, size_t want_to_alloc);

void* _merge_adjacent_blocks_realloc(void* p, size_t size) {
    MallocMetadata* p_metadata = _get_metadata(p);
    MallocMetadata* prev_metadata_on_heap = p_metadata->prev_on_heap;
    MallocMetadata* next_metadata_on_heap = p_metadata->next_on_heap;
    MallocMetadata* next_metadata_on_sort = p_metadata->next;
    MallocMetadata* prev_metadata_on_sort = p_metadata->prev;
    void* return_ptr = nullptr;
    MallocMetadata* return_ptr_metadata = nullptr;

    //Calculate offsets


    //Priority list for merging (detailed in srealloc)
    if (prev_metadata_on_heap && next_metadata_on_heap) { //Both sides available
        if (prev_metadata_on_heap->is_free && prev_metadata_on_heap->size + p_metadata->size + data.size_of_meta_data >= size) {//Merge Prev 1st
            prev_metadata_on_heap->size += p_metadata->size + data.size_of_meta_data;
            //On heap link
            prev_metadata_on_heap->next_on_heap = p_metadata->next_on_heap;
            next_metadata_on_heap->prev_on_heap = prev_metadata_on_heap;
            //List Unlinks
            //Unlink p_metadata
            return_ptr_metadata = prev_metadata_on_heap;
            _unlink_on_sort(prev_metadata_on_heap);
            _unlink_on_sort(p_metadata);
            return_ptr = _get_data_after_metadata(return_ptr_metadata);

            if (_should_split(return_ptr_metadata, size)) {
                _split(return_ptr_metadata, size);
            }

//            data.num_of_allocated_blocks --;
//            data.num_of_free_blocks --;
//            data.num_of_free_bytes += data.size_of_meta_data;
//            data.num_of_meta_data_bytes -= data.size_of_meta_data;
//            data.num_of_allocated_bytes += data.size_of_meta_data;
        }
        else if (next_metadata_on_heap->is_free && next_metadata_on_heap->size + p_metadata->size + data.size_of_meta_data >= size) { //Merge Next 2nd
            p_metadata->size += next_metadata_on_heap->size + data.size_of_meta_data;
            //On heap link
            p_metadata->next_on_heap = next_metadata_on_heap->next_on_heap;
            if(next_metadata_on_heap->next_on_heap) {
                next_metadata_on_heap->next_on_heap->prev_on_heap = p_metadata;
            }
            //Unlink next on sort
            return_ptr_metadata = p_metadata;
            _unlink_on_sort(p_metadata);
            _unlink_on_sort(next_metadata_on_heap);
            return_ptr = _get_data_after_metadata(return_ptr_metadata);

            if (_should_split(return_ptr_metadata, size)) {
                _split(return_ptr_metadata, size);
            }

//            data.num_of_allocated_blocks --;
//            data.num_of_free_blocks --;
//            data.num_of_free_bytes += data.size_of_meta_data;
//            data.num_of_meta_data_bytes -= data.size_of_meta_data;
//            data.num_of_allocated_bytes += data.size_of_meta_data;
        }
        else if (prev_metadata_on_heap->is_free && next_metadata_on_heap->is_free && next_metadata_on_heap->size +
                                                                                     prev_metadata_on_heap->size +
                                                                                     p_metadata->size +
                                                                                     2*data.size_of_meta_data >= size) { //Merge both last
            prev_metadata_on_heap->size += p_metadata->size + next_metadata_on_heap->size + 2*data.size_of_meta_data;
            //On heap link
            prev_metadata_on_heap->next_on_heap = next_metadata_on_heap->next_on_heap;
            if(next_metadata_on_heap->next_on_heap) {
                next_metadata_on_heap->next_on_heap->prev_on_heap = prev_metadata_on_heap;
            }
            //Unlink on sort
            return_ptr_metadata = prev_metadata_on_heap;
            _unlink_on_sort(prev_metadata_on_heap);
            _unlink_on_sort(p_metadata);
            _unlink_on_sort(next_metadata_on_heap);
            return_ptr = _get_data_after_metadata(return_ptr_metadata);


            if (_should_split(return_ptr_metadata, size)) {
                _split(return_ptr_metadata, size);
            }

//            data.num_of_allocated_blocks -= 2;
//            data.num_of_free_blocks -= 2;
//            data.num_of_free_bytes += 2*data.size_of_meta_data;
//            data.num_of_meta_data_bytes -= 2*data.size_of_meta_data;
//            data.num_of_allocated_bytes += 2*data.size_of_meta_data;
        }

        else {
            _mark_free(p_metadata);
            return_ptr = _get_data_after_metadata(p_metadata);
        }

        if (return_ptr_metadata) _reinsert(return_ptr_metadata);
        return return_ptr;
    }
        //One sided checks
    else if (prev_metadata_on_heap && prev_metadata_on_heap->is_free && prev_metadata_on_heap->size +
                                                                        p_metadata->size +
                                                                        data.size_of_meta_data >= size) {
        prev_metadata_on_heap->size += p_metadata->size + data.size_of_meta_data;
        prev_metadata_on_heap->next_on_heap = next_metadata_on_heap;
        //Unlink on sort
        return_ptr_metadata = prev_metadata_on_heap;
        _unlink_on_sort(prev_metadata_on_heap);
        _unlink_on_sort(p_metadata);
        return_ptr = _get_data_after_metadata(return_ptr_metadata);

        if (_should_split(return_ptr_metadata, size)) {
            _split(return_ptr_metadata, size);
        }

//        data.num_of_allocated_blocks --;
//        data.num_of_free_blocks --;
//        data.num_of_free_bytes += data.size_of_meta_data;
//        data.num_of_meta_data_bytes -= data.size_of_meta_data;
//        data.num_of_allocated_bytes += data.size_of_meta_data;
    }

    else if (next_metadata_on_heap && next_metadata_on_heap->is_free && next_metadata_on_heap->size +
                                                                        p_metadata->size +
                                                                        data.size_of_meta_data >= size) {
        p_metadata->size += next_metadata_on_heap->size + data.size_of_meta_data;
        p_metadata->next_on_heap = next_metadata_on_heap->next_on_heap;
        if (next_metadata_on_heap->next_on_heap) next_metadata_on_heap->next_on_heap->prev_on_heap = p_metadata;

        //Unlink on sort
        return_ptr_metadata = p_metadata;
        _unlink_on_sort(p_metadata);
        _unlink_on_sort(next_metadata_on_heap);
        return_ptr = _get_data_after_metadata(return_ptr_metadata);

        if (_should_split(return_ptr_metadata, size)) {
            _split(return_ptr_metadata, size);
        }

//        data.num_of_allocated_blocks --;
//        data.num_of_free_blocks --;
//        data.num_of_free_bytes += data.size_of_meta_data;
//        data.num_of_meta_data_bytes -= data.size_of_meta_data;
//        data.num_of_allocated_bytes += data.size_of_meta_data;
    }

    else {
        _mark_free(p_metadata);
        return_ptr = _get_data_after_metadata(p_metadata);
    }

    if (return_ptr_metadata) _reinsert(return_ptr_metadata);
    return return_ptr;
}

void* _merge_adjacent_blocks(void* p) {
    MallocMetadata* p_metadata = _get_metadata(p);
    MallocMetadata* prev_metadata_on_heap = p_metadata->prev_on_heap;
    MallocMetadata* next_metadata_on_heap = p_metadata->next_on_heap;
    MallocMetadata* next_metadata_on_sort = p_metadata->next;
    MallocMetadata* prev_metadata_on_sort = p_metadata->prev;
    void* return_ptr = nullptr;
    MallocMetadata* return_ptr_metadata = nullptr;

    //Calculate offsets


    //Priority list for merging (detailed in srealloc)
    if (prev_metadata_on_heap && next_metadata_on_heap) { //Both sides available
        if (prev_metadata_on_heap->is_free && !(next_metadata_on_heap->is_free)) {//Merge Prev 1st
            prev_metadata_on_heap->size += p_metadata->size + data.size_of_meta_data;
            //On heap link
            prev_metadata_on_heap->next_on_heap = p_metadata->next_on_heap;
            next_metadata_on_heap->prev_on_heap = prev_metadata_on_heap;
            //List Unlinks
            //Unlink p_metadata
            return_ptr_metadata = prev_metadata_on_heap;
            _unlink_on_sort(prev_metadata_on_heap);
            _unlink_on_sort(p_metadata);
            return_ptr = _get_data_after_metadata(return_ptr_metadata);


//            data.num_of_allocated_blocks --;
//            data.num_of_free_blocks --;
//            data.num_of_free_bytes += data.size_of_meta_data;
//            data.num_of_meta_data_bytes -= data.size_of_meta_data;
//            data.num_of_allocated_bytes += data.size_of_meta_data;
        }
        else if (!(prev_metadata_on_heap->is_free) && next_metadata_on_heap->is_free) { //Merge Next 2nd
            p_metadata->size += next_metadata_on_heap->size + data.size_of_meta_data;
            //On heap link
            p_metadata->next_on_heap = next_metadata_on_heap->next_on_heap;
            if(next_metadata_on_heap->next_on_heap) {
                next_metadata_on_heap->next_on_heap->prev_on_heap = p_metadata;
            }
            //Unlink next on sort
            return_ptr_metadata = p_metadata;
            _unlink_on_sort(p_metadata);
            _unlink_on_sort(next_metadata_on_heap);
            return_ptr = _get_data_after_metadata(return_ptr_metadata);

//            data.num_of_allocated_blocks --;
//            data.num_of_free_blocks --;
//            data.num_of_free_bytes += data.size_of_meta_data;
//            data.num_of_meta_data_bytes -= data.size_of_meta_data;
//            data.num_of_allocated_bytes += data.size_of_meta_data;
        }
        else if (prev_metadata_on_heap->is_free && next_metadata_on_heap->is_free) { //Merge both last
            prev_metadata_on_heap->size += p_metadata->size + next_metadata_on_heap->size + 2*data.size_of_meta_data;
            //On heap link
            prev_metadata_on_heap->next_on_heap = next_metadata_on_heap->next_on_heap;
            if(next_metadata_on_heap->next_on_heap) {
                next_metadata_on_heap->next_on_heap->prev_on_heap = prev_metadata_on_heap;
            }
            //Unlink on sort
            return_ptr_metadata = prev_metadata_on_heap;
            _unlink_on_sort(prev_metadata_on_heap);
            _unlink_on_sort(p_metadata);
            _unlink_on_sort(next_metadata_on_heap);
            return_ptr = _get_data_after_metadata(return_ptr_metadata);


//            data.num_of_allocated_blocks -= 2;
//            data.num_of_free_blocks -= 2;
//            data.num_of_free_bytes += 2*data.size_of_meta_data;
//            data.num_of_meta_data_bytes -= 2*data.size_of_meta_data;
//            data.num_of_allocated_bytes += 2*data.size_of_meta_data;
        }

        else {
            _mark_free(p_metadata);
            return_ptr = _get_data_after_metadata(p_metadata);
        }

        if (return_ptr_metadata) _reinsert(return_ptr_metadata);
        return return_ptr;
    }
    //One sided checks
    else if (prev_metadata_on_heap && prev_metadata_on_heap->is_free && !next_metadata_on_heap) {
        prev_metadata_on_heap->size += p_metadata->size + data.size_of_meta_data;
        prev_metadata_on_heap->next_on_heap = next_metadata_on_heap;
        //Unlink on sort
        return_ptr_metadata = prev_metadata_on_heap;
        _unlink_on_sort(prev_metadata_on_heap);
        _unlink_on_sort(p_metadata);
        return_ptr = _get_data_after_metadata(return_ptr_metadata);


//        data.num_of_allocated_blocks --;
//        data.num_of_free_blocks --;
//        data.num_of_free_bytes += data.size_of_meta_data;
//        data.num_of_meta_data_bytes -= data.size_of_meta_data;
//        data.num_of_allocated_bytes += data.size_of_meta_data;
    }

    else if (next_metadata_on_heap && next_metadata_on_heap->is_free && !prev_metadata_on_heap) {
        p_metadata->size += next_metadata_on_heap->size + data.size_of_meta_data;
        p_metadata->next_on_heap = next_metadata_on_heap->next_on_heap;
        if (next_metadata_on_heap->next_on_heap) next_metadata_on_heap->next_on_heap->prev_on_heap = p_metadata;

        //Unlink on sort
        return_ptr_metadata = p_metadata;
        _unlink_on_sort(p_metadata);
        _unlink_on_sort(next_metadata_on_heap);
        return_ptr = _get_data_after_metadata(return_ptr_metadata);


//        data.num_of_allocated_blocks --;
//        data.num_of_free_blocks --;
//        data.num_of_free_bytes += data.size_of_meta_data;
//        data.num_of_meta_data_bytes -= data.size_of_meta_data;
//        data.num_of_allocated_bytes += data.size_of_meta_data;
    }

    else {
        _mark_free(p_metadata);
        return_ptr = _get_data_after_metadata(p_metadata);
    }

    if (return_ptr_metadata) _reinsert(return_ptr_metadata);
    return return_ptr;
}

// Returns True if we should split the block, else False.
// p = pointer to a memory block on the heap.
// waot_to_alloc = how much bytes to allocate.
bool _should_split(void* p, size_t want_to_alloc) {

    if (p == nullptr) return false;
//    MallocMetadata* metadata = _get_metadata(p);
    size_t curr_size = ((MallocMetadata*)p)->size;
    size_t metadata_size = data.size_of_meta_data; //TODO: check this again for measurments
    int remaining_size = 0;

    remaining_size = curr_size - metadata_size - want_to_alloc;
    return (remaining_size >= MIN_SIZE);
}

// Returns number of pages, rounded up, for mmap allocation
int _calc_mmap_size(size_t size) {
    size_t pagesize = getpagesize();
    int number_of_pages = ceil(size / pagesize);
    return number_of_pages;
}




void _remove_from_mmap_list(void* p){
    MallocMetadata* metadata = _get_metadata(p);
//    MallocMetadata* temp = nullptr;   /// did not compile with this
    MallocMetadata* prev_ptr = metadata->prev; //prev->p->next
    MallocMetadata* next_ptr = metadata->next;

//    temp  = metadata; //for testing   /// did not compile with this

    prev_ptr->next = next_ptr;
    if(next_ptr) {
        next_ptr->prev = prev_ptr;
    }


    munmap(p,metadata->size + data.size_of_meta_data);


}

bool _isWildernessChunk(MallocMetadata* p_metadata) {
//    void* end_heap = sbrk(0);
//    size_t offset = data.size_of_meta_data + p_metadata->size;
//    void* ptr = (static_cast<char*>((void*)p_metadata) + offset);
//    return (ptr == end_heap);
    return !(p_metadata->next_on_heap);
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

    ptr = ((MallocMetadata*)heap_mmap)->next;   /// skipping dummy
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

    ptr = ((MallocMetadata*)heap_mmap)->next;   /// skipping dummy
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

    ptr = ((MallocMetadata*)heap_mmap)->next;
    while(ptr) {
        data.num_of_meta_data_bytes += data.size_of_meta_data;
        ptr = ptr->next;
    }
    /// storing in this var for debug
    return data.num_of_meta_data_bytes;
}

static bool _check_list_link(void* start) {
    MallocMetadata* ptr = ((MallocMetadata*)start)->next_on_heap;
    MallocMetadata* last_ptr = nullptr;
    size_t counter = _num_allocated_blocks();
    while ((ptr)){
        counter--;
        if (ptr == tail) {
            return true;
        } else if (ptr > ptr->next_on_heap && ptr->next_on_heap != nullptr) {
            return false;
        } else if (ptr < ptr->prev_on_heap) {
            return false;
        } else {
            last_ptr = ptr;
            ptr = ptr->next_on_heap;
        }
    }

//    return true;
    return counter == 0;

}

static MallocMetadata* _getWildernessBlock() {
    MallocMetadata* ptr = ((MallocMetadata*)heap_sbrk)->next_on_heap;
    while(ptr) {
        if (ptr->next_on_heap == nullptr) {
            break;
        }
        ptr = ptr->next_on_heap;
    }

    return ptr;
}

//size_t _num_free_blocks() {return data.num_of_free_blocks;}
//size_t _num_free_bytes() {return data.num_of_free_bytes;}
//size_t _num_allocated_blocks() {return data.num_of_allocated_blocks;}
//size_t _num_allocated_bytes() {return data.num_of_allocated_bytes;}
//size_t _size_meta_data() {return data.size_of_meta_data;}
//size_t _num_meta_data_bytes() {return data.num_of_meta_data_bytes;}

void* smalloc (size_t size) {
    if (size == 0 || size > MAX_SIZE) return nullptr;
    MallocMetadata* new_metadata = nullptr;
    void* new_data = nullptr;
    MallocMetadata* ptr = nullptr;
    MallocMetadata* last_block = nullptr;

    bool is_mmap = false;
    if (size >= MIN_MMAP_SIZE) {
        ptr = (MallocMetadata*)heap_mmap;
        is_mmap = true;
    } else {
        ptr = (MallocMetadata*)heap_sbrk;
    }

    // list is sorted and iterate through the sbrk list
    while (!is_mmap && ptr) {

        /// Check if the block is big enough
        if (ptr->size >= size && ptr->is_free) {
            // check if the block can be splitted i.e after the split the unused part
            // has at least 128 bytes exluding metadata
            if (_should_split(ptr, size)) {
                _split(ptr, size);
            }
            _mark_used(ptr);
            void* new_ptr = _get_data_after_metadata(ptr);
            return new_ptr;
        }
        else {
            last_block = ptr;
            ptr = ptr->next;
        }
    }

    MallocMetadata* wilderness = _getWildernessBlock();
    //Wilderness territory
    size_t diff = 0;

    if (_num_allocated_blocks() > 0 && !is_mmap && wilderness) {
        if (wilderness->is_free) {
            void* old_biggest_block_start = _get_data_after_metadata(wilderness);

            size_t  old_size = wilderness->size;
            diff = size - old_size;
//        void* temp = _aux_smalloc(diff);  /// did not compile with this
            _aux_smalloc(diff);                 /// so changed to this

            wilderness->is_free = false;
            wilderness->next_on_heap = nullptr;
            wilderness->size = size;

            _unlink_on_sort(wilderness);
            _reinsert(wilderness);

            return old_biggest_block_start;
        }
    }

//    if(data.num_of_allocated_blocks > 0   &&
//      (!is_mmap && last_block->is_free)   &&
//      _isWildernessChunk(last_block)) {
//
//        void* old_biggest_block_start = _get_data_after_metadata(last_block);
//
//        size_t  old_size = last_block->size;
//        diff = size - old_size;
////        void* temp = _aux_smalloc(diff);  /// did not compile with this
//        _aux_smalloc(diff);                 /// so changed to this
//
//        last_block->is_free = false;
//        last_block->next = nullptr;
//        last_block->size = size;
//
//        return old_biggest_block_start;
//    }



    // Adding new member to either list
    if(is_mmap) {   // mmap list
        //size_t page_size = getpagesize();
        //size_t metadata_size_in_pages = _calc_mmap_size(data.size_of_meta_data);
        //metadata_size_in_pages = metadata_size_in_pages * page_size;
        //int number_of_pages = _calc_mmap_size(size+data.size_of_meta_data);
        //size_t data_and_meta_size_in_pages = number_of_pages * page_size;
        void* temp_ptr = nullptr;
        temp_ptr = mmap(nullptr,
                        size+data.size_of_meta_data,
                        PROT_WRITE | PROT_READ,
                        MAP_ANONYMOUS | MAP_PRIVATE,
                        -1,
                        0);
        new_metadata = (MallocMetadata*)temp_ptr;
        new_data = _get_data_after_metadata(new_metadata);
        new_metadata->is_free = false;
        new_metadata->size = size;
        new_metadata->is_mmapped = true;
        _add_allocation(new_metadata, (MallocMetadata*)heap_mmap);
    } else {        //heap/sbrk list
        new_metadata = (MallocMetadata*)_aux_smalloc(data.size_of_meta_data);
        tail->next_on_heap = new_metadata;
        new_metadata->prev_on_heap = tail;
        new_metadata->next_on_heap = nullptr;
        tail = new_metadata;
        new_data = _aux_smalloc(size);
        new_metadata->size = size;
        new_metadata->is_free = false;
        _add_allocation(new_metadata, (MallocMetadata*)heap_sbrk);
    }


    return new_data;
}

void* scalloc(size_t num, size_t size) {
    /// not checkin size < 0 because size_t = unsigned int
    if (size == 0 || num*size > MAX_SIZE) return nullptr;

    size_t calloc_size = num * size;
    void* ptr = smalloc(calloc_size);
    if (ptr != nullptr) {
        memset(ptr, 0, calloc_size);
    }

    return ptr;
}

void sfree(void* p) {
    if (p == nullptr) return;

    MallocMetadata* p_metadata = _get_metadata(p);
    bool is_mmap = p_metadata->is_mmapped;
    if (p_metadata->is_free && !is_mmap) return;

    if(is_mmap) {
        _remove_from_mmap_list(p);
    } else {
        _mark_free(p_metadata);
        _merge_adjacent_blocks(p);
    }
}

void* srealloc(void* oldp, size_t size) {
    if (size == 0 || size > MAX_SIZE) return nullptr;

    if (oldp == nullptr) return smalloc(size);
    MallocMetadata* oldp_metadata = _get_metadata(oldp);

    /// taking care of wilderness chunk
    if (!(oldp_metadata->is_mmapped) && oldp_metadata->size < size && _isWildernessChunk(oldp_metadata)) {
        size_t diff = 0;

        diff = size - oldp_metadata->size;
//        void* temp = _aux_smalloc(diff);  /// did not compile with this
        _aux_smalloc(diff);                 /// so changed to this
        oldp_metadata->is_free = false;
        oldp_metadata->size = size;

        _unlink_on_sort(oldp_metadata);
        _reinsert(oldp_metadata);

        //Update data
//        data.num_of_allocated_bytes += diff;
        return oldp;
    }

    if (oldp_metadata->is_mmapped) {
        if (oldp_metadata->size == size) return oldp;
        size_t min_size = 0;
        if (oldp_metadata->size > size) min_size = size;
        else min_size = oldp_metadata->size;

        void* ptr = smalloc(size);

        if (ptr != nullptr) {
            memcpy(ptr, oldp, min_size);
            sfree(oldp);
        }

        return ptr;
    }

    // this is a fucking heap situation
    if (oldp_metadata->size >= size) {
        if (_should_split(oldp_metadata, size)) {
            _split(oldp_metadata, size);
        }
        return oldp;   /// reusing block
    }

    size_t old_size = oldp_metadata->size;
    // void* ptr = _get_data_after_metadata(oldp_metadata);
//    void* ptr = _merge_adjacent_blocks(oldp);   /// b, c, d are taken care of here.
    void* ptr = _merge_adjacent_blocks_realloc(oldp, size);
    MallocMetadata* new_metadata = _get_metadata(ptr);

    if (old_size < new_metadata->size) {
        /// if we are here, we enlarged the block.
        if (new_metadata->size >= size) {
            /// sub-section 2
            if (_should_split(new_metadata, size)) {
                _split(new_metadata, size);
            }
            if (ptr != nullptr) {
                memmove(ptr, oldp, size);
//            std::cout << "good job!" << std::endl;
                new_metadata->is_free = false;
            }
        }
        else {
            ptr = smalloc(size);    /// e is taken care of here
            if (ptr != nullptr) {
                memmove(ptr, oldp, size);
//            std::cout << "good job!" << std::endl;
                new_metadata->is_free = false;
                sfree(oldp);
            }
        }
    }
    else {
        /// if we are here, we did not merge.
        ptr = smalloc(size);

        if (ptr != nullptr) {
            memmove(ptr, oldp, oldp_metadata->size);
            new_metadata->is_free = false;
            sfree(oldp);
        }
    }

    return ptr;
}

/*Functions for OShw4Test*/
MallocMetadata* _get_tail(MallocMetadata* list_dummy) {
    MallocMetadata* ptr = nullptr;
    MallocMetadata* last_ptr = nullptr;

    ptr = list_dummy;
    while(ptr) {
        last_ptr = ptr;
        ptr = ptr->next;
    }

    return last_ptr;
}

MallocMetadata* _get_head(MallocMetadata* list_dummy) {
    if (list_dummy == nullptr) return nullptr;
    return list_dummy->next;
}

///=========================================malloc_3_tests.cpp======================================================///
//#define META_SIZE         sizeof(MallocMetadata) // put your meta data name here.
//#define  MAX_MALLOC_SIZE 1e8
//#include <assert.h>
//
//int main() {
//
//    //   global_list_init = NULL; init of global list.
//    //   global_list = NULL;
//
//    assert(smalloc(0) == NULL);
//    assert(smalloc(MAX_MALLOC_SIZE + 1) == NULL);
//
//    // allocate 6 big blocks
//    void *b1, *b2, *b3, *b4, *b5, *b6, *b7, *b8, *b9;
//    b1 = smalloc(1000);
//    assert(_num_free_blocks() == 0);
//    assert(_num_free_bytes() == 0);
//    assert(_num_allocated_blocks() == 1);
//    assert(_num_allocated_bytes() == 1000);
//    assert(_num_meta_data_bytes() == META_SIZE);
//
//    b2 = smalloc(2000);
//    assert(_num_free_blocks() == 0);
//    assert(_num_free_bytes() == 0);
//    assert(_num_allocated_blocks() == 2);
//    assert(_num_allocated_bytes() == 3000);
//    assert(_num_meta_data_bytes() == 2 * META_SIZE);
//
//    b3 = smalloc(3000);
//    assert(_num_free_blocks() == 0);
//    assert(_num_free_bytes() == 0);
//    assert(_num_allocated_blocks() == 3);
//    assert(_num_allocated_bytes() == 6000);
//    assert(_num_meta_data_bytes() == 3 * META_SIZE);
//
//    b4 = smalloc(4000);
//    assert(_num_free_blocks() == 0);
//    assert(_num_free_bytes() == 0);
//    assert(_num_allocated_blocks() == 4);
//    assert(_num_allocated_bytes() == 10000);
//    assert(_num_meta_data_bytes() == 4 * META_SIZE);
//
//    b5 = smalloc(5000);
//    assert(_num_free_blocks() == 0);
//    assert(_num_free_bytes() == 0);
//    assert(_num_allocated_blocks() == 5);
//    assert(_num_allocated_bytes() == 15000);
//    assert(_num_meta_data_bytes() == 5 * META_SIZE);
//
//    b6 = smalloc(6000);
//    assert(_num_free_blocks() == 0);
//    assert(_num_free_bytes() == 0);
//    assert(_num_allocated_blocks() == 6);
//    assert(_num_allocated_bytes() == 21000);
//    assert(_num_meta_data_bytes() == 6 * META_SIZE);
//
//    sfree(b3);
//    assert(_num_free_blocks() == 1);
//    assert(_num_free_bytes() == 3000);
//    assert(_num_allocated_blocks() == 6);
//    assert(_num_allocated_bytes() == 21000);
//    assert(_num_meta_data_bytes() == 6 * META_SIZE);
//
//    b3 = smalloc(1000);
//    assert(_num_free_blocks() == 1);
//    assert(_num_free_bytes() == 3000 - META_SIZE - 1000);
//    assert(_num_allocated_blocks() == 7);
//    assert(_num_allocated_bytes() == 21000 - META_SIZE);
//    assert(_num_meta_data_bytes() == 7 * META_SIZE);
//
//    // free of b3 will combine two free blocks back together.
//    sfree(b3);
//    assert(_num_free_blocks() == 1);
//    assert(_num_free_bytes() == 3000);
//    assert(_num_allocated_blocks() == 6);
//    assert(_num_allocated_bytes() == 21000);
//    assert(_num_meta_data_bytes() == 6 * META_SIZE);
//
//    // checking if free combine works on the other way.
//    // (a a f a a a /free b4/ a a f f a a /combine b3 and b4/ a a f a a)
//    sfree(b4);
//    assert(_num_free_blocks() == 1);
//    assert(_num_free_bytes() == 7000 + META_SIZE);
//    assert(_num_allocated_blocks() == 5);
//    assert(_num_allocated_bytes() == 21000 + META_SIZE);
//    assert(_num_meta_data_bytes() == 5 * META_SIZE);
//
//    // split will not work now, size is lower than 128 of data EXCLUDING the size of your meta-data structure.
//    // the free block is of size 7000 + META_SIZE, so the splittable part is 127.
//    b3 = smalloc(7000 - 127);
//    assert(_num_free_blocks() == 0);
//    assert(_num_free_bytes() == 0);
//    assert(_num_allocated_blocks() == 5);
//    assert(_num_allocated_bytes() == 21000 + META_SIZE);
//    assert(_num_meta_data_bytes() == 5 * META_SIZE);
//
//    // free will work and combine.
//    sfree(b3);
//    assert(_num_free_blocks() == 1);
//    assert(_num_free_bytes() == 7000 + META_SIZE);
//    assert(_num_allocated_blocks() == 5);
//    assert(_num_allocated_bytes() == 21000 + META_SIZE);
//    assert(_num_meta_data_bytes() == 5 * META_SIZE);
//
//    // split will work now, size is 128 of data EXCLUDING the size of your meta-data structure.
//    // the free block is of size 7000 + META_SIZE, so the splittable part is 127.
//    b3 = smalloc(7000 - 128);
//    assert(_num_free_blocks() == 1);
//    assert(_num_free_bytes() == 128);
//    assert(_num_allocated_blocks() == 6);
//    assert(_num_allocated_bytes() == 21000);
//    assert(_num_meta_data_bytes() == 6 * META_SIZE);
//
//    // checking wilderness chunk
//    sfree(b6);
//    assert(_num_free_blocks() == 2);
//    assert(_num_free_bytes() == 128 + 6000);
//    assert(_num_allocated_blocks() == 6);
//    assert(_num_allocated_bytes() == 21000);
//    assert(_num_meta_data_bytes() == 6 * META_SIZE);
//
//    // should take the middle block of size 128.
//    b7 = smalloc(128);
//    assert(_num_free_blocks() == 1);
//    assert(_num_free_bytes() == 6000);
//    assert(_num_allocated_blocks() == 6);
//    assert(_num_allocated_bytes() == 21000);
//    assert(_num_meta_data_bytes() == 6 * META_SIZE);
//
//    // should split wilderness block.   /// <-- I think this is not true so it makes problems from now on....
//    b6 = smalloc(5000);
//    assert(_num_free_blocks() == 1);
//    assert(_num_free_bytes() == 1000 - META_SIZE);
//    assert(_num_allocated_blocks() == 7);
//    assert(_num_allocated_bytes() == 21000 - META_SIZE);
//    assert(_num_meta_data_bytes() == 7 * META_SIZE);
//
//    // should enlarge wilderness block.
//    b8 = smalloc(1000);
//    assert(_num_free_blocks() == 0);
//    assert(_num_free_bytes() == 0);
//    assert(_num_allocated_blocks() == 7);
//    assert(_num_allocated_bytes() == 21000);
//    assert(_num_meta_data_bytes() == 7 * META_SIZE);
//
//    // add another block to the end.
//    b9 = smalloc(10);
//    assert(_num_free_blocks() == 0);
//    assert(_num_free_bytes() == 0);
//    assert(_num_allocated_blocks() == 8);
//    assert(_num_allocated_bytes() == 21012);
//    assert(_num_meta_data_bytes() == 8 * META_SIZE);
//
//    //
//    sfree(b9);
//    assert(_num_free_blocks() == 1);
//    assert(_num_free_bytes() == 12);
//    assert(_num_allocated_blocks() == 8);
//    assert(_num_allocated_bytes() == 21012);
//    assert(_num_meta_data_bytes() == 8 * META_SIZE);
//
//    b9 = smalloc(8);
//    assert(_num_free_blocks() == 0);
//    assert(_num_free_bytes() == 0);
//    assert(_num_allocated_blocks() == 8);
//    assert(_num_allocated_bytes() == 21012);
//    assert(_num_meta_data_bytes() == 8 * META_SIZE);
//
//    // checking alignment
//    b9 = smalloc(1);
//    assert(_num_free_blocks() == 0);
//    assert(_num_free_bytes() == 0);
//    assert(_num_allocated_blocks() == 9);
//    assert(_num_allocated_bytes() == 21016);
//    assert(_num_meta_data_bytes() == 9 * META_SIZE);
//
//    b9 = smalloc(2);
//    assert(_num_free_blocks() == 0);
//    assert(_num_free_bytes() == 0);
//    assert(_num_allocated_blocks() == 10);
//    assert(_num_allocated_bytes() == 21020);
//    assert(_num_meta_data_bytes() == 10 * META_SIZE);
//
//    b9 = smalloc(3);
//    assert(_num_free_blocks() == 0);
//    assert(_num_free_bytes() == 0);
//    assert(_num_allocated_blocks() == 11);
//    assert(_num_allocated_bytes() == 21024);
//    assert(_num_meta_data_bytes() == 11 * META_SIZE);
//
//    b9 = smalloc(4);
//    assert(_num_free_blocks() == 0);
//    assert(_num_free_bytes() == 0);
//    assert(_num_allocated_blocks() == 12);
//    assert(_num_allocated_bytes() == 21028);
//    assert(_num_meta_data_bytes() == 12 * META_SIZE);
//
//    return 0;
//}
///==================================================================================================================///

///=========================================malloc_3_tests_realloc.cpp===============================================///
//#include <cstdio>
//#include <assert.h>
//
//#define META_SIZE         data.size_of_meta_data // put your meta data name here.
//
//int main() {
//
//    //   global_list_init = NULL; init of global list.
//    //   global_list = NULL;
//
//    // allocate blocks
//    void *b1, *b2, *b3, *b4, *b5, *b6, *b7, *b8, *b9, *b10, *b11, *b12, *b13, *b14 ,*b15;
//
//    // allocate first block, heap: b1(1000)
//    b1 = smalloc(1000);
//    assert(_num_free_blocks() == 0);
//    assert(_num_free_bytes() == 0);
//    assert(_num_allocated_blocks() == 1);
//    assert(_num_allocated_bytes() == 1000);
//    assert(_num_meta_data_bytes() == META_SIZE);
//
//    // check realloc with NULL, heap: 1000(no_free), 2000(not_free)
//    b2 = srealloc(NULL, 2000);
//    assert(_num_free_blocks() == 0);
//    assert(_num_free_bytes() == 0);
//    assert(_num_allocated_blocks() == 2);
//    assert(_num_allocated_bytes() == 3000);
//    assert(_num_meta_data_bytes() == 2 * META_SIZE);
//
//    // check realloc with same size, should return the same block, heap: 1000(no_free), 2000(not_free)
//    b3 = srealloc(b2, 2000);
//    assert(b2 == b3);
//    assert(_num_free_blocks() == 0);
//    assert(_num_free_bytes() == 0);
//    assert(_num_allocated_blocks() == 2);
//    assert(_num_allocated_bytes() == 3000);
//    assert(_num_meta_data_bytes() == 2 * META_SIZE);
//
//    // check realloc with bigger size on wilderness block, heap: 1000(no_free), 3000(not_free)
//    b4 = srealloc(b2, 3000);
//    assert(b2 == b4);
//    assert(_num_free_blocks() == 0);
//    assert(_num_free_bytes() == 0);
//    assert(_num_allocated_blocks() == 2);
//    assert(_num_allocated_bytes() == 4000);
//    assert(_num_meta_data_bytes() == 2 * META_SIZE);
//
//    // check realloc for smaller size and not enough space for split, heap: 1000(no_free), 3000(not_free)
//    b5 = srealloc(b1, 900);
//    assert(b1 == b5);
//    assert(_num_free_blocks() == 0);
//    assert(_num_free_bytes() == 0);
//    assert(_num_allocated_blocks() == 2);
//    assert(_num_allocated_bytes() == 4000);
//    assert(_num_meta_data_bytes() == 2 * META_SIZE);
//
//    // heap: 1000(no_free), 3000(not_free), 2000(not_free)
//    b6 = smalloc(2000);
//    assert(_num_free_blocks() == 0);
//    assert(_num_free_bytes() == 0);
//    assert(_num_allocated_blocks() == 3);
//    assert(_num_allocated_bytes() == 6000);
//    assert(_num_meta_data_bytes() == 3 * META_SIZE);
//
//    // check realloc for smaller size and enough space for split
//    // heap: 1000(no_free), 1000(not_free), 2000 - META_SIZE(free), 2000(not_free)
//    b7 = srealloc(b2, 1000);
//    assert(_num_free_blocks() == 1);
//    assert(_num_free_bytes() == 2000 - META_SIZE);
//    assert(_num_allocated_blocks() == 4);
//    assert(_num_allocated_bytes() == 6000 - META_SIZE);
//    assert(_num_meta_data_bytes() == 4 * META_SIZE);
//
//    // check realloc for smaller size and enough space for split, after that merge with upper free block
//    // heap: 1000(no_free), 500(not_free), 2500 - META_SIZE(free), 2000(not_free)
//    b8 = srealloc(b7, 500);     /// <-- do we really need to merge here???
/////                                             instructions says "marked free and added to the list" and not merge...
/////                                             it only says thar sfree is merging...
//    assert(_num_free_blocks() == 1);
//    assert(_num_free_bytes() == 2500 - META_SIZE);
//    assert(_num_allocated_blocks() == 4);
//    assert(_num_allocated_bytes() == 6000 - META_SIZE);
//    assert(_num_meta_data_bytes() == 4 * META_SIZE);
//
//    // check realloc for bigger size, upper block is free and enough space for split
//    // heap: 1000(no_free), 1000(not_free), 2000 - META_SIZE(free), 2000(not_free)
//    b9 = srealloc(b8, 1000);
//    assert(_num_free_blocks() == 1);
//    assert(_num_free_bytes() == 2000 - META_SIZE);
//    assert(_num_allocated_blocks() == 4);
//    assert(_num_allocated_bytes() == 6000 - META_SIZE);
//    assert(_num_meta_data_bytes() == 4 * META_SIZE);
//
//    // check realloc for bigger size, upper block is free and not enough space for split
//    // heap: 1000(no_free), 3000(not_free), 2000(not_free)
//    b10 = realloc(b9, 3000);
//    assert(b10 == b9);
//    assert(_num_free_blocks() == 0);
//    assert(_num_free_bytes() == 0);
//    assert(_num_allocated_blocks() == 3);
//    assert(_num_allocated_bytes() == 6000);
//    assert(_num_meta_data_bytes() == 3 * META_SIZE);
//
//    // check realloc for bigger size, upper block is free and lower block is free and size requested fits the 3 blocks merged
//    // heap: 1000(no_free), 1000(not_free), 2000 - META_SIZE(free), 2000(not_free)
//    b11 = srealloc(b10, 1000);
//    assert(_num_free_blocks() == 1);
//    assert(_num_free_bytes() == 2000 - META_SIZE);
//    assert(_num_allocated_blocks() == 4);
//    assert(_num_allocated_bytes() == 6000 - META_SIZE);
//    assert(_num_meta_data_bytes() == 4 * META_SIZE);
//
//    // heap: 1000(free), 1000(not_free), 2000 - META_SIZE(free), 2000(not_free)
//    sfree(b1);
//    assert(_num_free_blocks() == 2);
//    assert(_num_free_bytes() == 3000 - META_SIZE);
//    assert(_num_allocated_blocks() == 4);
//    assert(_num_allocated_bytes() == 6000 - META_SIZE);
//    assert(_num_meta_data_bytes() == 4 * META_SIZE);
//
//    // heap: 4000 + META_SIZE(not_free), 2000(not_free)
//    b12 = srealloc(b10, 4000);
//    assert(b12 == b1);
//    assert(_num_free_blocks() == 0);
//    assert(_num_free_bytes() == 0);
//    assert(_num_allocated_blocks() == 2);
//    assert(_num_allocated_bytes() == 6000 + META_SIZE);
//    assert(_num_meta_data_bytes() == 2 * META_SIZE);
//
//    printf("GOOD JOB!\n");
//
//    return 0;
//}

///==================================================================================================================///


//#define META_SIZE         sizeof(MallocMetadata) // put your meta data name here.
//int main() {
//    void *p1, *p2, *p3, *p4, *p5, *p6, *p7, *p8, *p9, *p10, *p11, *p12, *p13, *p14 ,*p15;
//    //check mmap allocation and malloc/calloc fails
//    smalloc(0);
//    smalloc(100000001);
//    scalloc(10000000, 10000000);
//    scalloc(100000001, 1);
//    scalloc(1, 100000001);
//    scalloc(50, 0);
//    scalloc(0, 50);
//
//    p1 = smalloc(100000000);
//    p2 = scalloc(1, 10000000);
//    p3 = scalloc(10000000, 1);
//    p1 = srealloc(p1, 100000000);
//    p3 = srealloc(p3, 20000000);
//    p3 = srealloc(p3, 10000000);
//    p4 = srealloc(nullptr, 10000000);
//    assert(_num_free_blocks() == 0);
//    assert(_num_free_bytes() == 0);
//    assert(_num_allocated_blocks() == 4);
//    assert(_num_allocated_bytes() == 100000000 + 30000000);
//    assert(_num_meta_data_bytes() == 4 * META_SIZE);
//    sfree(p1), sfree(p2), sfree(p3), sfree(p4);
//    assert(_num_free_blocks() == 0);
//    assert(_num_free_bytes() == 0);
//    assert(_num_allocated_blocks() == 0);
//    assert(_num_allocated_bytes() == 0);
//    assert(_num_meta_data_bytes() == 0);
//    p1 = smalloc(1000);
//    p2 = smalloc(1000);
//    p3 = smalloc(1000);
//    p4 = scalloc(500,2);
//    p5 = scalloc(2, 500);
//    assert(_check_list_link(heap_sbrk));
//    assert(_num_free_blocks() == 0);
//    assert(_num_free_bytes() == 0);
//    assert(_num_allocated_blocks() == 5);
//    assert(_num_allocated_bytes() == 5000);
//    assert(_num_meta_data_bytes() == 5 * META_SIZE);
//    //check free, combine and split
//    sfree(p3);
//    assert(_check_list_link(heap_sbrk));
//    assert(_num_free_blocks() == 1);
//    assert(_num_free_bytes() == 1000);
//    assert(_num_allocated_blocks() == 5);
//    assert(_num_allocated_bytes() == 5000);
//    assert(_num_meta_data_bytes() == 5 * META_SIZE);
//    sfree(p5);
//    assert(_check_list_link(heap_sbrk));
//    assert(_num_free_blocks() == 2);
//    assert(_num_free_bytes() == 2000);
//    assert(_num_allocated_blocks() == 5);
//    assert(_num_allocated_bytes() == 5000);
//    assert(_num_meta_data_bytes() == 5 * META_SIZE);
//    sfree(p4); //Here it should be merged with 2 blocks
//    assert(_check_list_link(heap_sbrk));
//    assert(_num_free_blocks() == 1);
//    assert(_num_free_bytes() == 3096);
//    assert(_num_allocated_blocks() == 3);
//    assert(_num_allocated_bytes() == 5096);
//    assert(_num_meta_data_bytes() == 3 * META_SIZE);
//    p3 = smalloc(1000);
//    assert(_check_list_link(heap_sbrk));
//    assert(_num_free_blocks() == 1);
//    assert(_num_free_bytes() == 2048);
//    assert(_num_allocated_blocks() == 4);
//    assert(_num_allocated_bytes() == 5048);
//    assert(_num_meta_data_bytes() == 4 * META_SIZE);
//    p4 = smalloc(1000);
//    assert(_check_list_link(heap_sbrk));
//    assert(_num_free_blocks() == 1);
//    assert(_num_free_bytes() == 1000);
//    assert(_num_allocated_blocks() == 5);
//    assert(_num_allocated_bytes() == 5000);
//    assert(_num_meta_data_bytes() == 5 * META_SIZE);
//    p5 = smalloc (1000);
//    assert(_check_list_link(heap_sbrk));
//    assert(_num_free_blocks() == 0);
//    assert(_num_free_bytes() == 0);
//    assert(_num_allocated_blocks() == 5);
//    assert(_num_allocated_bytes() == 5000);
//    assert(_num_meta_data_bytes() == 5 * META_SIZE);
//    sfree(p4);
//    assert(_check_list_link(heap_sbrk));
//    assert(_num_free_blocks() == 1);
//    assert(_num_free_bytes() == 1000);
//    assert(_num_allocated_blocks() == 5);
//    assert(_num_allocated_bytes() == 5000);
//    assert(_num_meta_data_bytes() == 5 * META_SIZE);
//    sfree(p5); //Merge with previous
//    assert(_check_list_link(heap_sbrk));
//    assert(_num_free_blocks() == 1);
//    assert(_num_free_bytes() == 2000 + META_SIZE);
//    assert(_num_allocated_blocks() == 4);
//    assert(_num_allocated_bytes() == 5000+META_SIZE);
//    assert(_num_meta_data_bytes() == 4 * META_SIZE);
//    sfree(p1);
//    assert(_check_list_link(heap_sbrk));
//    assert(_num_free_blocks() == 2);
//    assert(_num_free_bytes() == 3000 + META_SIZE);
//    assert(_num_allocated_blocks() == 4);
//    assert(_num_allocated_bytes() == 5000 + META_SIZE);
//    assert(_num_meta_data_bytes() == 4 * META_SIZE);
//    sfree(p1); //Nothing should happen
//    assert(_check_list_link(heap_sbrk));
//    assert(_num_free_blocks() == 2);
//    assert(_num_free_bytes() == 3000 + META_SIZE);
//    assert(_num_allocated_blocks() == 4);
//    assert(_num_allocated_bytes() == 5000 + META_SIZE);
//    assert(_num_meta_data_bytes() == 4 * META_SIZE);
//    sfree(p2); //Merge with prev
//    assert(_check_list_link(heap_sbrk));
//    assert(_num_free_blocks() == 2);
//    assert(_num_free_bytes() == 4000+2*META_SIZE);
//    assert(_num_allocated_blocks() == 3);
//    assert(_num_allocated_bytes() == 5000 + 2*META_SIZE);
//    assert(_num_meta_data_bytes() == 3 * META_SIZE);
//    p1 = smalloc(1000);
//    assert(_check_list_link(heap_sbrk));
//    assert(_num_free_blocks() == 2);
//    assert(_num_free_bytes() == 3000+META_SIZE);
//    assert(_num_allocated_blocks() == 4);
//    assert(_num_allocated_bytes() == 5000 + META_SIZE);
//    assert(_num_meta_data_bytes() == 4 * META_SIZE);
//    p2 = smalloc(1000);
//    assert(_check_list_link(heap_sbrk));
//    assert(_num_free_blocks() == 1);
//    assert(_num_free_bytes() == 2000+META_SIZE);
//    assert(_num_allocated_blocks() == 4);
//    assert(_num_allocated_bytes() == 5000 + META_SIZE);
//    assert(_num_meta_data_bytes() == 4 * META_SIZE);
//    sfree(p2);
//    assert(_check_list_link(heap_sbrk));
//    assert(_num_free_blocks() == 2);
//    assert(_num_free_bytes() == 3000+META_SIZE);
//    assert(_num_allocated_blocks() == 4);
//    assert(_num_allocated_bytes() == 5000 + META_SIZE);
//    assert(_num_meta_data_bytes() == 4 * META_SIZE);
//    sfree(p1); //Merge with next
//    assert(_check_list_link(heap_sbrk));
//    assert(_num_free_blocks() == 2);
//    assert(_num_free_bytes() == 4000 + 2*META_SIZE);
//    assert(_num_allocated_blocks() == 3);
//    assert(_num_allocated_bytes() == 5000 + 2*META_SIZE);
//    assert(_num_meta_data_bytes() == 3 * META_SIZE);
//    //assert(_get_tail((MallocMetadata*)heap_sbrk) == (MallocMetadata*)p4 - 1);
//    //assert(_get_head((MallocMetadata*)heap_sbrk) == (MallocMetadata*)p1 - 1);
//
//
//    //list condition: FREE OF 2032 -> BLOCK OF 1000 -> FREE OF 2032
//    p1 = smalloc(1000);
//    p2 = smalloc(1000);
//    p4 = scalloc(1500,2);
//    assert(_check_list_link(heap_sbrk));
//    assert(_num_free_blocks() == 0);
//    assert(_num_free_bytes() == 0);
//    assert(_num_allocated_blocks() == 4);
//    assert(_num_allocated_bytes() == 6000);
//    assert(_num_meta_data_bytes() == 4 * META_SIZE);
//    //assert(_get_tail((MallocMetadata*)heap_sbrk) == (MallocMetadata*)p4 - 1);
//    //assert(_get_head((MallocMetadata*)heap_sbrk) == (MallocMetadata*)p1 - 1);
//    sfree(p1); sfree(p2); sfree(p3); sfree(p4);
//    assert(_check_list_link(heap_sbrk));
//    assert(_num_free_blocks() == 1);
//    assert(_num_free_bytes() == 6000+3*META_SIZE);
//    assert(_num_allocated_blocks() == 1);
//    assert(_num_allocated_bytes() == 6000+3*META_SIZE);
//    assert(_num_meta_data_bytes() == META_SIZE);
//    //assert(_get_tail((MallocMetadata*)heap_sbrk) == (MallocMetadata*)p1 - 1);
//    //assert(_get_head((MallocMetadata*)heap_sbrk) == (MallocMetadata*)p1 - 1);
//    //assert(_get_head((MallocMetadata*)heap_mmap) == nullptr);
//    //assert(_get_head((MallocMetadata*)heap_mmap) == nullptr);
//    p1 = smalloc(1000);     /// before: 6144    after: 1000-->5096
//    assert(_check_list_link(heap_sbrk));
//    assert(_num_free_blocks() == 1);
//    assert(_num_free_bytes() == 5000+2*META_SIZE);
//    assert(_num_allocated_blocks() == 2);
//    assert(_num_allocated_bytes() == 6000+2*META_SIZE);
//    assert(_num_meta_data_bytes() == 2*META_SIZE);
//
//    p2 = scalloc(2,500);    /// before: 1000-->5096    after: 1000-->1000-->4048
//    assert(_check_list_link(heap_sbrk));
////    assert(_num_free_blocks() == 1);
////    assert(_num_free_bytes() == 5000+2*META_SIZE);
////    assert(_num_allocated_blocks() == 2);
////    assert(_num_allocated_bytes() == 6000+2*META_SIZE);
////    assert(_num_meta_data_bytes() == 2*META_SIZE);
//
//    p3 = smalloc(1000);     /// before: 1000-->1000-->4048    after: 1000-->1000-->1000-->3000
//    assert(_check_list_link(heap_sbrk));
////    assert(_num_free_blocks() == 1);
////    assert(_num_free_bytes() == 1000);
////    assert(_num_allocated_blocks() == 6);
////    assert(_num_allocated_bytes() == 7000);
////    assert(_num_meta_data_bytes() == 6*META_SIZE);
//
//    p4 = scalloc(10,100);   /// before: 1000-->1000-->1000-->3000    after: 1000-->1000-->1000-->1000-->1952
//    assert(_check_list_link(heap_sbrk));
////    assert(_num_free_blocks() == 1);
////    assert(_num_free_bytes() == 1000);
////    assert(_num_allocated_blocks() == 6);
////    assert(_num_allocated_bytes() == 7000);
////    assert(_num_meta_data_bytes() == 6*META_SIZE);
//
//    p5 = smalloc(1000);     /// before: 1000-->1000-->1000-->1000-->1952    after: 904-->1000-->1000-->1000-->1000-->1000
//    assert(_check_list_link(heap_sbrk));
////    assert(_num_free_blocks() == 1);
////    assert(_num_free_bytes() == 1000);
////    assert(_num_allocated_blocks() == 6);
////    assert(_num_allocated_bytes() == 7000);
////    assert(_num_meta_data_bytes() == 6*META_SIZE);
//
//    p6 = scalloc(250,4);    /// before: 904-->1000-->1000-->1000-->1000-->1000    after: 904-->1000-->1000-->1000-->1000-->1000
//    assert(_check_list_link(heap_sbrk));
////    assert(_num_free_blocks() == 1);
////    assert(_num_free_bytes() == 1000);
////    assert(_num_allocated_blocks() == 6);
////    assert(_num_allocated_bytes() == 7000);
////    assert(_num_meta_data_bytes() == 6*META_SIZE);
//
//    //list condition: SIX BLOCKS, 1000 BYTES EACH
//    //now we'll check realloc
//
//    //check englare
//    sfree(p5);
//    for (int i = 0; i < 250; ++i)
//        *((int*)p6+i) = 2;
//
//    assert(_check_list_link(heap_sbrk));
//    p6 = srealloc(p6, 2000);
//    for (int i = 0; i < 250; ++i)
//        assert(*((int*)p6+i) == 2);
//    assert(_num_free_blocks() == 1);
//    assert(_num_free_bytes() == 1000);
//    assert(_num_allocated_blocks() == 6);
//    assert(_num_allocated_bytes() == 7000);
//    assert(_num_meta_data_bytes() == 6*META_SIZE);
//    //assert(_get_tail((MallocMetadata*)heap_sbrk) == (MallocMetadata*)p6 - 1);
//    //assert(_get_head((MallocMetadata*)heap_sbrk) == (MallocMetadata*)p1 - 1);
//
//    p5 = smalloc(1000);
//    p1 = srealloc(p1,500-sizeof(MallocMetadata));
//    sfree(p1);
//    //check case a
//    assert(((MallocMetadata*)p5-1)->is_free==false);
//    sfree(p5); sfree(p3);
//    //check case b
//    p3 = srealloc(p4,2000);
//    //LIST CONDITION: 1-> 1000 FREE, 2->1000, 3->2000,
//    // 5-> 1000 FREE, 6->2000 FREE
//    //now i return the list to its state before tha last realloc
//    p3 = srealloc(p3,1000);
//    p4 = ((char*)p3 + 1000 + META_SIZE);
//    p1 = smalloc(1000);
//    p4=smalloc(1000);
//    sfree(p1);
//    sfree(p3);
//    //check case d
//    p3 = srealloc(p4,3000);
//    assert(_num_free_blocks() == 1);
//    assert(_num_free_bytes() == 1000);
//    assert(_num_allocated_blocks() == 4);
//    assert(_num_allocated_bytes() == 7000 + 2*META_SIZE);
//    assert(_num_meta_data_bytes() == 4*META_SIZE);
//    //just change the state of the list
//    p3 = srealloc(p3,500);
//    p1 = smalloc(1000);
//    p4 = smalloc(1500);
//    p5 = smalloc(1000);
//    sfree(p1); sfree(p3); sfree(p5);
//    //check case c
//    p4 = srealloc(p4, 2500);
//    p4 = srealloc(p4, 1000);
//    assert(_num_free_blocks() == 3);
//    assert(_num_free_bytes() == 3000);
//    assert(_num_allocated_blocks() == 6);
//    assert(_num_allocated_bytes() == 7000);
//    assert(_num_meta_data_bytes() == 6*META_SIZE);
//    //just change the state of the list
//    p1 = smalloc(1000);
//    p3 = smalloc(500);
//    sfree(p1);
//    //check case e
//    p1 = srealloc(p3, 900);
//    assert(_num_free_blocks() == 2);
//    assert(_num_free_bytes() == 2000);
//    assert(_num_allocated_blocks() == 6);
//    assert(_num_allocated_bytes() == 7000);
//    assert(_num_meta_data_bytes() == 6*META_SIZE);
////    assert(_get_tail((MallocMetadata*)heap_sbrk) == (MallocMetadata*)p6 - 1);
////    assert(_get_head((MallocMetadata*)heap_sbrk) == (MallocMetadata*)p1 - 1);
//    //check case f
//    for (int i = 0; i < 250; ++i)
//        *((int*)p1+i) = 2;
//    p7 = srealloc(p1,2000);
//    for (int i = 0; i < 250; ++i)
//        assert(*((int*)p7+i) == 2);
//    assert(_num_free_blocks() == 3);
//    assert(_num_free_bytes() == 3000);
//    assert(_num_allocated_blocks() == 7);
//    assert(_num_allocated_bytes() == 9000);
//    assert(_num_meta_data_bytes() == 7*META_SIZE);
////    assert(_get_tail((MallocMetadata*)heap_sbrk) == (MallocMetadata*)p7 - 1);
////    assert(_get_head((MallocMetadata*)heap_sbrk) == (MallocMetadata*)p1 - 1);
//    //block_list.print_list();
//    /*LIST CONDITION:
//     * 1->1000 free
//     * 2->1000
//     * 3->500 free
//     * 4->1000
//     * 5->1500 free
//     * 6->2000
//     * 7->2000
//    */
//    std::cout << "good job!" << std::endl;
//    return 0;
//}

//
//#define META_SIZE         sizeof(MallocMetadata) // put your meta data name here.
//
//int main() {
//
//    //   global_list_init = NULL; init of global list.
//    //   global_list = NULL;
//
//    assert(smalloc(0) == NULL);
//    assert(smalloc(MAX_SIZE + 1) == NULL);
//
//    // allocate 6 big blocks
//    void *b1, *b2, *b3, *b4, *b5, *b6, *b7, *b8, *b9;
//    b1 = smalloc(1000);
//    assert(_num_free_blocks() == 0);
//    assert(_num_free_bytes() == 0);
//    assert(_num_allocated_blocks() == 1);
//    assert(_num_allocated_bytes() == 1000);
//    assert(_num_meta_data_bytes() == META_SIZE);
//
//    b2 = smalloc(2000);
//    assert(_num_free_blocks() == 0);
//    assert(_num_free_bytes() == 0);
//    assert(_num_allocated_blocks() == 2);
//    assert(_num_allocated_bytes() == 3000);
//    assert(_num_meta_data_bytes() == 2 * META_SIZE);
//
//    b3 = smalloc(3000);
//    assert(_num_free_blocks() == 0);
//    assert(_num_free_bytes() == 0);
//    assert(_num_allocated_blocks() == 3);
//    assert(_num_allocated_bytes() == 6000);
//    assert(_num_meta_data_bytes() == 3 * META_SIZE);
//
//    b4 = smalloc(4000);
//    assert(_num_free_blocks() == 0);
//    assert(_num_free_bytes() == 0);
//    assert(_num_allocated_blocks() == 4);
//    assert(_num_allocated_bytes() == 10000);
//    assert(_num_meta_data_bytes() == 4 * META_SIZE);
//
//    b5 = smalloc(5000);
//    assert(_num_free_blocks() == 0);
//    assert(_num_free_bytes() == 0);
//    assert(_num_allocated_blocks() == 5);
//    assert(_num_allocated_bytes() == 15000);
//    assert(_num_meta_data_bytes() == 5 * META_SIZE);
//
//    b6 = smalloc(6000);
//    assert(_num_free_blocks() == 0);
//    assert(_num_free_bytes() == 0);
//    assert(_num_allocated_blocks() == 6);
//    assert(_num_allocated_bytes() == 21000);
//    assert(_num_meta_data_bytes() == 6 * META_SIZE);
//
//    sfree(b3);
//    assert(_num_free_blocks() == 1);
//    assert(_num_free_bytes() == 3000);
//    assert(_num_allocated_blocks() == 6);
//    assert(_num_allocated_bytes() == 21000);
//    assert(_num_meta_data_bytes() == 6 * META_SIZE);
//
//    b3 = smalloc(1000);
//    assert(_num_free_blocks() == 1);
//    assert(_num_free_bytes() == 3000 - META_SIZE - 1000);
//    assert(_num_allocated_blocks() == 7);
//    assert(_num_allocated_bytes() == 21000 - META_SIZE);
//    assert(_num_meta_data_bytes() == 7 * META_SIZE);
//
//    // free of b3 will combine two free blocks back together.
//    sfree(b3);
//    assert(_num_free_blocks() == 1);
//    assert(_num_free_bytes() == 3000);
//    assert(_num_allocated_blocks() == 6);
//    assert(_num_allocated_bytes() == 21000);
//    assert(_num_meta_data_bytes() == 6 * META_SIZE);
//
//    // checking if free combine works on the other way.
//    // (a a f a a a /free b4/ a a f f a a /combine b3 and b4/ a a f a a)
//    sfree(b4);
//    assert(_num_free_blocks() == 1);
//    assert(_num_free_bytes() == 7000 + META_SIZE);
//    assert(_num_allocated_blocks() == 5);
//    assert(_num_allocated_bytes() == 21000 + META_SIZE);
//    assert(_num_meta_data_bytes() == 5 * META_SIZE);
//
//    // split will not work now, size is lower than 128 of data EXCLUDING the size of your meta-data structure.
//    // the free block is of size 7000 + META_SIZE, so the splittable part is 127.
//    b3 = smalloc(7000 - 127);
//    assert(_num_free_blocks() == 0);
//    assert(_num_free_bytes() == 0);
//    assert(_num_allocated_blocks() == 5);
//    assert(_num_allocated_bytes() == 21000 + META_SIZE);
//    assert(_num_meta_data_bytes() == 5 * META_SIZE);
//
//    // free will work and combine.
//    sfree(b3);
//    assert(_num_free_blocks() == 1);
//    assert(_num_free_bytes() == 7000 + META_SIZE);
//    assert(_num_allocated_blocks() == 5);
//    assert(_num_allocated_bytes() == 21000 + META_SIZE);
//    assert(_num_meta_data_bytes() == 5 * META_SIZE);
//
//    // split will work now, size is 128 of data EXCLUDING the size of your meta-data structure.
//    // the free block is of size 7000 + META_SIZE, so the splittable part is 127.
//    b3 = smalloc(7000 - 128);
//    assert(_num_free_blocks() == 1);
//    assert(_num_free_bytes() == 128);
//    assert(_num_allocated_blocks() == 6);
//    assert(_num_allocated_bytes() == 21000);
//    assert(_num_meta_data_bytes() == 6 * META_SIZE);
//
//    // checking wilderness chunk
//    sfree(b6);
//    assert(_num_free_blocks() == 2);
//    assert(_num_free_bytes() == 128 + 6000);
//    assert(_num_allocated_blocks() == 6);
//    assert(_num_allocated_bytes() == 21000);
//    assert(_num_meta_data_bytes() == 6 * META_SIZE);
//
//    // should take the middle block of size 128.
//    b7 = smalloc(128);
//    assert(_num_free_blocks() == 1);
//    assert(_num_free_bytes() == 6000);
//    assert(_num_allocated_blocks() == 6);
//    assert(_num_allocated_bytes() == 21000);
//    assert(_num_meta_data_bytes() == 6 * META_SIZE);
//
//    // should split wilderness block.
//    b6 = smalloc(5000);
//    assert(_num_free_blocks() == 1);
//    assert(_num_free_bytes() == 1000 - META_SIZE);
//    assert(_num_allocated_blocks() == 7);
//    assert(_num_allocated_bytes() == 21000 - META_SIZE);
//    assert(_num_meta_data_bytes() == 7 * META_SIZE);
//
//    // should enlarge wilderness block.
//    b8 = smalloc(1000);
//    assert(_num_free_blocks() == 0);
//    assert(_num_free_bytes() == 0);
//    assert(_num_allocated_blocks() == 7);
//    assert(_num_allocated_bytes() == 21000);
//    assert(_num_meta_data_bytes() == 7 * META_SIZE);
//
//    // add another block to the end.
////    b9 = smalloc(10);
//    b9 = smalloc(12);
//    assert(_num_free_blocks() == 0);
//    assert(_num_free_bytes() == 0);
//    assert(_num_allocated_blocks() == 8);
//    assert(_num_allocated_bytes() == 21012);
//    assert(_num_meta_data_bytes() == 8 * META_SIZE);
//
//    //
//    sfree(b9);
//    assert(_num_free_blocks() == 1);
//    assert(_num_free_bytes() == 12);
//    assert(_num_allocated_blocks() == 8);
//    assert(_num_allocated_bytes() == 21012);
//    assert(_num_meta_data_bytes() == 8 * META_SIZE);
//
//    b9 = smalloc(8);
//    assert(_num_free_blocks() == 0);
//    assert(_num_free_bytes() == 0);
//    assert(_num_allocated_blocks() == 8);
//    assert(_num_allocated_bytes() == 21012);
//    assert(_num_meta_data_bytes() == 8 * META_SIZE);
//
////    // checking alignment
////    b9 = smalloc(1);
////    assert(_num_free_blocks() == 0);
////    assert(_num_free_bytes() == 0);
////    assert(_num_allocated_blocks() == 9);
////    assert(_num_allocated_bytes() == 21016);
////    assert(_num_meta_data_bytes() == 9 * META_SIZE);
////
////    b9 = smalloc(2);
////    assert(_num_free_blocks() == 0);
////    assert(_num_free_bytes() == 0);
////    assert(_num_allocated_blocks() == 10);
////    assert(_num_allocated_bytes() == 21020);
////    assert(_num_meta_data_bytes() == 10 * META_SIZE);
////
////    b9 = smalloc(3);
////    assert(_num_free_blocks() == 0);
////    assert(_num_free_bytes() == 0);
////    assert(_num_allocated_blocks() == 11);
////    assert(_num_allocated_bytes() == 21024);
////    assert(_num_meta_data_bytes() == 11 * META_SIZE);
////
////    b9 = smalloc(4);
////    assert(_num_free_blocks() == 0);
////    assert(_num_free_bytes() == 0);
////    assert(_num_allocated_blocks() == 12);
////    assert(_num_allocated_bytes() == 21028);
////    assert(_num_meta_data_bytes() == 12 * META_SIZE);
//
//    return 0;
//}
