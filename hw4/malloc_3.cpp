#include<unistd.h>
#include<string.h>

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

void* _aux_smalloc(size_t);
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
///=======================================Aux functions=======================================///
static void _mark_free(MallocMetadata* to_mark) {
    to_mark->is_free = true;

    /// update data
    data.num_of_free_blocks++;
    data.num_of_free_bytes += to_mark->size;
}

static void _mark_used(MallocMetadata* to_mark) {
    to_mark->is_free = false;

    /// update data
    data.num_of_free_blocks--;
    data.num_of_free_bytes -= to_mark->size;
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
            prev_ptr->next = to_add;
            ptr->prev = to_add;
            break;
        }

        /// if sizes are equal, sort by addrs.
        else {
            if (ptr < to_add) {
                to_add->next = ptr->next;
                to_add->prev = ptr;
                ptr->next = to_add;
                break;
            }
            else if (ptr > to_add) {
                to_add->next = ptr;
                to_add->prev = ptr->prev;
                ptr->prev = to_add;
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
    data.num_of_allocated_blocks++;
    data.num_of_allocated_bytes += to_add->size;
    data.num_of_meta_data_bytes += data.size_of_meta_data;
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

static MallocMetadata* _get_metadata(void* p) {
    MallocMetadata* p_metadata = (MallocMetadata*)(static_cast<char*>(p) - data.size_of_meta_data);
    return p_metadata;
}

void _split(void* p, size_t size) {
    MallocMetadata* p_metadata = _get_metadata(p);
    size_t new_block_size = p_metadata->size - size;
    size_t offset = data.size_of_meta_data + size;
    MallocMetadata* new_metadata = (MallocMetadata*)(static_cast<char*>(p) + offset);
    new_metadata->size = new_block_size - data.size_of_meta_data;
    new_metadata->is_free = true;
    new_metadata->is_mmapped = false;
    p_metadata->size = size;

    /// which head we need to send???
    _add_allocation(new_metadata, (MallocMetadata*)heap_sbrk);
    data.num_of_allocated_bytes -= new_metadata->size + data.size_of_meta_data;
    data.num_of_free_bytes -= data.size_of_meta_data;
}

void _merge_adjacent_blocks(void* p) {
    MallocMetadata* p_metadata = _get_metadata(p);
    MallocMetadata* prev_metadata = p_metadata->prev;
    MallocMetadata* next_metadata = p_metadata->next;
    if (prev_metadata && next_metadata) {
        if (prev_metadata->is_free && next_metadata->is_free) {
            prev_metadata->size += p_metadata->size + next_metadata->size + 2*data.size_of_meta_data;
            prev_metadata->next = next_metadata->next;
            if (next_metadata->next->prev) next_metadata->next->prev = prev_metadata;

            data.num_of_allocated_blocks -= 2;
            data.num_of_free_blocks -= 2;
            data.num_of_free_bytes += p_metadata->size + 2*data.size_of_meta_data;
            data.num_of_meta_data_bytes -= 2*data.size_of_meta_data;
        }
        else if (prev_metadata->is_free && !(next_metadata->is_free)) {
            prev_metadata->size += p_metadata->size + data.size_of_meta_data;
            prev_metadata->next = p_metadata->next;
            p_metadata->next->prev = prev_metadata;

            data.num_of_allocated_blocks --;
            data.num_of_free_blocks --;
            data.num_of_free_bytes += p_metadata->size + data.size_of_meta_data;
            data.num_of_meta_data_bytes -= data.size_of_meta_data;
        }
        else if (!(prev_metadata->is_free) && next_metadata->is_free) {
            p_metadata->size += next_metadata->size + data.size_of_meta_data;
            p_metadata->next = next_metadata->next;
            p_metadata->next->prev = p_metadata;

            data.num_of_allocated_blocks --;
            data.num_of_free_blocks --;
            data.num_of_free_bytes += next_metadata->size + data.size_of_meta_data;
            data.num_of_meta_data_bytes -= data.size_of_meta_data;
        }
        else {
            _mark_free(p_metadata);
        }

        return;
    }

    else if (prev_metadata && prev_metadata->is_free && !next_metadata) {
        prev_metadata->size += p_metadata->size + data.size_of_meta_data;
        prev_metadata->next = p_metadata->next;

        data.num_of_allocated_blocks --;
        data.num_of_free_blocks --;
        data.num_of_free_bytes += p_metadata->size + data.size_of_meta_data;
        data.num_of_meta_data_bytes -= data.size_of_meta_data;
    }

    else if (next_metadata && next_metadata->is_free && !prev_metadata) {
        p_metadata->size += next_metadata->size + data.size_of_meta_data;
        p_metadata->next = next_metadata->next;
        if (next_metadata->next) next_metadata->next->prev = p_metadata;

        data.num_of_allocated_blocks --;
        data.num_of_free_blocks --;
        data.num_of_free_bytes += next_metadata->size + data.size_of_meta_data;
        data.num_of_meta_data_bytes -= data.size_of_meta_data;
    }

    else {
        _mark_free(p_metadata);
    }

    return;
}
///===========================================================================================///


size_t _num_free_blocks() {return data.num_of_free_blocks;}
size_t _num_free_bytes() {return data.num_of_free_bytes;}
size_t _num_allocated_blocks() {return data.num_of_allocated_blocks;}
size_t _num_allocated_bytes() {return data.num_of_allocated_bytes;}
size_t _size_meta_data() {return data.size_of_meta_data;}


void* smalloc (size_t size) {
    if (size == 0 || size > MAX_SIZE) return nullptr;
    MallocMetadata* new_addrs = nullptr;
    MallocMetadata* ptr = (MallocMetadata*)heap;

    while (ptr) {
       if (ptr->size >= size && ptr->is_free) {
           _mark_used(ptr);
           ptr += data.size_of_meta_data;
           return ptr;
       }
       else {
            ptr = ptr->next;
       }
    }

    /// does the last element still points to end of heap???
    new_addrs = (MallocMetadata*)_aux_smalloc(size);
    new_addrs->size = size;
    new_addrs->is_free = false;
    _add_allocation(new_addrs);
    new_addrs += data.size_of_meta_data;
    return new_addrs;
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
    if (p_metadata->is_free) return;
    _mark_free(p_metadata);
}

void* srealloc(void* oldp, size_t size) {
    if (size == 0 || size > MAX_SIZE) return nullptr;

    if (oldp == nullptr) return smalloc(size);
    MallocMetadata* oldp_metadata = _get_metadata(oldp);
    if (oldp_metadata->size >= size) return oldp;

    void* ptr = smalloc(size);

    if (ptr != nullptr) {
        memcpy(ptr, oldp, oldp_metadata->size);
        sfree(oldp);    
    }

    return ptr;
}