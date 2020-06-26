#include<unistd.h>
#include<string.h>
#include <sys/mman.h>
#include <math.h>

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

static void* _get_data_after_metadata(MallocMetadata* metadata) {
    unsigned char* byteptr = reinterpret_cast<unsigned char*>(metadata);
    MallocMetadata* p = (MallocMetadata*)(static_cast<char*>((void*)metadata) + data.size_of_meta_data);
    return (void*)p;
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

    /// which head we need to send??? Roman: Only heap since mmap are deleted when freed so are never split
    /// 
    _add_allocation(new_metadata, (MallocMetadata*)heap_sbrk);
    data.num_of_allocated_bytes -= new_metadata->size + data.size_of_meta_data;
    data.num_of_free_bytes -= data.size_of_meta_data;
}

void* _merge_adjacent_blocks(void* p) {
    MallocMetadata* p_metadata = _get_metadata(p);
    MallocMetadata* prev_metadata = p_metadata->prev;
    MallocMetadata* next_metadata = p_metadata->next;
    void* return_ptr = nullptr;

    //Priority list for merging (detailed in srealloc)
    if (prev_metadata && next_metadata) { //Both sides available
        if (prev_metadata->is_free && !(next_metadata->is_free)) {//Merge Prev 1st
            prev_metadata->size += p_metadata->size + data.size_of_meta_data;
            prev_metadata->next = p_metadata->next;
            p_metadata->next->prev = prev_metadata;
            return_ptr = _get_data_after_metadata(prev_metadata);

            data.num_of_allocated_blocks --;
            data.num_of_free_blocks --;
            data.num_of_free_bytes += p_metadata->size + data.size_of_meta_data;
            data.num_of_meta_data_bytes -= data.size_of_meta_data;
        }
        else if (!(prev_metadata->is_free) && next_metadata->is_free) { //Merge Next 2nd
            p_metadata->size += next_metadata->size + data.size_of_meta_data;
            p_metadata->next = next_metadata->next;
            p_metadata->next->prev = p_metadata;
            return_ptr = _get_data_after_metadata(p_metadata);

            data.num_of_allocated_blocks --;
            data.num_of_free_blocks --;
            data.num_of_free_bytes += next_metadata->size + data.size_of_meta_data;
            data.num_of_meta_data_bytes -= data.size_of_meta_data;
        }
        else if (prev_metadata->is_free && next_metadata->is_free) { //Merge both last
            prev_metadata->size += p_metadata->size + next_metadata->size + 2*data.size_of_meta_data;
            prev_metadata->next = next_metadata->next;
            if (next_metadata->next->prev) next_metadata->next->prev = prev_metadata;
            return_ptr = _get_data_after_metadata(prev_metadata);

            data.num_of_allocated_blocks -= 2;
            data.num_of_free_blocks -= 2;
            data.num_of_free_bytes += p_metadata->size + 2*data.size_of_meta_data;
            data.num_of_meta_data_bytes -= 2*data.size_of_meta_data;
        }
        
        else {
            _mark_free(p_metadata);
            return_ptr = _get_data_after_metadata(p_metadata);
        }

        return return_ptr;
    }
    //One sided checks
    else if (prev_metadata && prev_metadata->is_free && !next_metadata) {
        prev_metadata->size += p_metadata->size + data.size_of_meta_data;
        prev_metadata->next = p_metadata->next;
        return_ptr = _get_data_after_metadata(prev_metadata);

        data.num_of_allocated_blocks --;
        data.num_of_free_blocks --;
        data.num_of_free_bytes += p_metadata->size + data.size_of_meta_data;
        data.num_of_meta_data_bytes -= data.size_of_meta_data;
    }

    else if (next_metadata && next_metadata->is_free && !prev_metadata) {
        p_metadata->size += next_metadata->size + data.size_of_meta_data;
        p_metadata->next = next_metadata->next;
        if (next_metadata->next) next_metadata->next->prev = p_metadata;
        return_ptr = _get_data_after_metadata(p_metadata);

        data.num_of_allocated_blocks --;
        data.num_of_free_blocks --;
        data.num_of_free_bytes += next_metadata->size + data.size_of_meta_data;
        data.num_of_meta_data_bytes -= data.size_of_meta_data;
    }

    else {
        _mark_free(p_metadata);
        return_ptr = _get_data_after_metadata(p_metadata);
    }

    return return_ptr;
}

// Returns True if we should split the block, else False.
// p = pointer to a memory block on the heap.
// waot_to_alloc = how much bytes to allocate.
bool _should_split(void* p, size_t want_to_alloc) {

    if (p == nullptr) return false;
    MallocMetadata* metadata = _get_metadata(p);   
    size_t curr_size = metadata->size;
    size_t metadata_size = sizeof(MallocMetadata); //TODO: check this again for measurments
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
    MallocMetadata* temp = nullptr;
    MallocMetadata* prev_ptr = metadata->prev; //prev->p->next
    MallocMetadata* next_ptr = metadata->next;

    int page_size = getpagesize();
    int num_pages_to_delete = _calc_mmap_size(metadata->size + data.size_of_meta_data);
    num_pages_to_delete = num_pages_to_delete * page_size; // num*4096

    temp  = metadata; //for testing
    prev_ptr->next = next_ptr; 
    next_ptr->prev = prev_ptr;

    munmap(p,num_pages_to_delete);


}

 ///===========================================================================================///


size_t _num_free_blocks() {return data.num_of_free_blocks;}
size_t _num_free_bytes() {return data.num_of_free_bytes;}
size_t _num_allocated_blocks() {return data.num_of_allocated_blocks;}
size_t _num_allocated_bytes() {return data.num_of_allocated_bytes;}
size_t _size_meta_data() {return data.size_of_meta_data;}

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
        
        /// Check if the blog is big enough
        if (ptr->size >= size && ptr->is_free) {
            // check if the block can be splitted i.e after the split the unused part
            // has at least 128 bytes exluding metadata
            if (_should_split(ptr, size)) {
                _split(ptr, size);
            }
            _mark_used(ptr);
            ptr += data.size_of_meta_data; //TODO: check
            return ptr;
        }
        else {
            last_block = ptr;
            ptr = ptr->next;
        }
    }

    //Wilderness territory
    size_t diff = 0;
    if(data.num_of_allocated_blocks > 0 && last_block->is_free) {
        MallocMetadata* biggest_block_meta = _get_metadata(last_block);
        void* old_biggest_block_start = _get_data_after_metadata(biggest_block_meta);

        diff = size - last_block->size;
        void* temp = _aux_smalloc(diff);

        biggest_block_meta->is_free = false;
        biggest_block_meta->next = nullptr;
        biggest_block_meta->size = size;

        //Update data
        data.num_of_free_bytes -= size;
        data.num_of_free_blocks--;
        data.num_of_allocated_bytes += diff;
        return old_biggest_block_start;      
    }

    

    // Adding new member to either list
    if(is_mmap) {   // mmap list
        size_t page_size = getpagesize();
        size_t metadata_size_in_pages = _calc_mmap_size(data.size_of_meta_data);
        metadata_size_in_pages = metadata_size_in_pages * page_size;
        int number_of_pages = _calc_mmap_size(size);
        size_t data_size_in_pages = number_of_pages * page_size;
        void* temp_ptr = nullptr;
        temp_ptr = mmap(nullptr, 
                        data_size_in_pages + metadata_size_in_pages, 
                        PROT_WRITE | PROT_READ, 
                        MAP_ANONYMOUS | MAP_PRIVATE,
                        -1,
                        0);
        new_metadata = (MallocMetadata*)temp_ptr;
        new_data = _get_data_after_metadata(new_metadata);

        _add_allocation(new_metadata, (MallocMetadata*)heap_mmap);
    } else {        //heap/sbrk list
        new_metadata = (MallocMetadata*)_aux_smalloc(sizeof(MallocMetadata));
        new_data = _aux_smalloc(size);
        _add_allocation(new_metadata, (MallocMetadata*)heap_sbrk);
    }

    new_metadata->size = size;
    new_metadata->is_free = false;
       
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
    if (oldp_metadata->next == nullptr && !(oldp_metadata->is_mmapped) && oldp_metadata->size < size) {
        size_t diff = 0;

        diff = size - oldp_metadata->size;
        void* temp = _aux_smalloc(diff);
        oldp_metadata->is_free = false;
        oldp_metadata->size = size;

        //Update data
        data.num_of_allocated_bytes += diff;
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
    if (oldp_metadata->size >= size) return oldp;   /// reusing block
    
    // void* ptr = _get_data_after_metadata(oldp_metadata);
    void* ptr = _merge_adjacent_blocks(oldp);   /// b, c, d are taken care of here.
    MallocMetadata* new_metadata = _get_metadata(ptr);

    if (oldp_metadata->size < new_metadata->size) {
        /// if we are here, we enlarged the block.
        if (new_metadata->size >= size) {
            /// sub-section 2
            if (_should_split(ptr, size)) {
                _split(ptr, size);
            }
        }
        else {
            ptr = smalloc(size);    /// e is taken care of here
        }
        
        if (ptr != nullptr) {
            memmove(ptr, oldp, size);
            new_metadata->is_free = false;
            sfree(oldp);
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