
#define META_SIZE         sizeof(Meta_Data) // put your meta data name here.

int main() {

 //   global_list_init = NULL; init of global list.
 //   global_list = NULL;

    assert(smalloc(0) == NULL);
    assert(smalloc(MAX_MALLOC_SIZE + 1) == NULL);

    // allocate 6 big blocks
    void *b1, *b2, *b3, *b4, *b5, *b6, *b7, *b8, *b9;
    b1 = smalloc(1000);
    assert(_num_free_blocks() == 0);
    assert(_num_free_bytes() == 0);
    assert(_num_allocated_blocks() == 1);
    assert(_num_allocated_bytes() == 1000);
    assert(_num_meta_data_bytes() == META_SIZE);

    b2 = smalloc(2000);
    assert(_num_free_blocks() == 0);
    assert(_num_free_bytes() == 0);
    assert(_num_allocated_blocks() == 2);
    assert(_num_allocated_bytes() == 3000);
    assert(_num_meta_data_bytes() == 2 * META_SIZE);

    b3 = smalloc(3000);
    assert(_num_free_blocks() == 0);
    assert(_num_free_bytes() == 0);
    assert(_num_allocated_blocks() == 3);
    assert(_num_allocated_bytes() == 6000);
    assert(_num_meta_data_bytes() == 3 * META_SIZE);

    b4 = smalloc(4000);
    assert(_num_free_blocks() == 0);
    assert(_num_free_bytes() == 0);
    assert(_num_allocated_blocks() == 4);
    assert(_num_allocated_bytes() == 10000);
    assert(_num_meta_data_bytes() == 4 * META_SIZE);

    b5 = smalloc(5000);
    assert(_num_free_blocks() == 0);
    assert(_num_free_bytes() == 0);
    assert(_num_allocated_blocks() == 5);
    assert(_num_allocated_bytes() == 15000);
    assert(_num_meta_data_bytes() == 5 * META_SIZE);

    b6 = smalloc(6000);
    assert(_num_free_blocks() == 0);
    assert(_num_free_bytes() == 0);
    assert(_num_allocated_blocks() == 6);
    assert(_num_allocated_bytes() == 21000);
    assert(_num_meta_data_bytes() == 6 * META_SIZE);

    free(b3);
    assert(_num_free_blocks() == 1);
    assert(_num_free_bytes() == 3000);
    assert(_num_allocated_blocks() == 6);
    assert(_num_allocated_bytes() == 21000);
    assert(_num_meta_data_bytes() == 6 * META_SIZE);

    b3 = smalloc(1000);
    assert(_num_free_blocks() == 1);
    assert(_num_free_bytes() == 3000 - META_SIZE - 1000);
    assert(_num_allocated_blocks() == 7);
    assert(_num_allocated_bytes() == 21000 - META_SIZE);
    assert(_num_meta_data_bytes() == 7 * META_SIZE);

    // free of b3 will combine two free blocks back together.
    free(b3);
    assert(_num_free_blocks() == 1);
    assert(_num_free_bytes() == 3000);
    assert(_num_allocated_blocks() == 6);
    assert(_num_allocated_bytes() == 21000);
    assert(_num_meta_data_bytes() == 6 * META_SIZE);

    // checking if free combine works on the other way.
    // (a a f a a a /free b4/ a a f f a a /combine b3 and b4/ a a f a a)
    free(b4);
    assert(_num_free_blocks() == 1);
    assert(_num_free_bytes() == 7000 + META_SIZE);
    assert(_num_allocated_blocks() == 5);
    assert(_num_allocated_bytes() == 21000 + META_SIZE);
    assert(_num_meta_data_bytes() == 5 * META_SIZE);

    // split will not work now, size is lower than 128 of data EXCLUDING the size of your meta-data structure.
    // the free block is of size 7000 + META_SIZE, so the splittable part is 127.
    b3 = smalloc(7000 - 127);
    assert(_num_free_blocks() == 0);
    assert(_num_free_bytes() == 0);
    assert(_num_allocated_blocks() == 5);
    assert(_num_allocated_bytes() == 21000 + META_SIZE);
    assert(_num_meta_data_bytes() == 5 * META_SIZE);

    // free will work and combine.
    free(b3);
    assert(_num_free_blocks() == 1);
    assert(_num_free_bytes() == 7000 + META_SIZE);
    assert(_num_allocated_blocks() == 5);
    assert(_num_allocated_bytes() == 21000 + META_SIZE);
    assert(_num_meta_data_bytes() == 5 * META_SIZE);

    // split will work now, size is 128 of data EXCLUDING the size of your meta-data structure.
    // the free block is of size 7000 + META_SIZE, so the splittable part is 127.
    b3 = smalloc(7000 - 128);
    assert(_num_free_blocks() == 1);
    assert(_num_free_bytes() == 128);
    assert(_num_allocated_blocks() == 6);
    assert(_num_allocated_bytes() == 21000);
    assert(_num_meta_data_bytes() == 6 * META_SIZE);

    // checking wilderness chunk
    free(b6);
    assert(_num_free_blocks() == 2);
    assert(_num_free_bytes() == 128 + 6000);
    assert(_num_allocated_blocks() == 6);
    assert(_num_allocated_bytes() == 21000);
    assert(_num_meta_data_bytes() == 6 * META_SIZE);

    // should take the middle block of size 128.
    b7 = smalloc(128);
    assert(_num_free_blocks() == 1);
    assert(_num_free_bytes() == 6000);
    assert(_num_allocated_blocks() == 6);
    assert(_num_allocated_bytes() == 21000);
    assert(_num_meta_data_bytes() == 6 * META_SIZE);

    // should split wilderness block.
    b6 = smalloc(5000);
    assert(_num_free_blocks() == 1);
    assert(_num_free_bytes() == 1000 - META_SIZE);
    assert(_num_allocated_blocks() == 7);
    assert(_num_allocated_bytes() == 21000 - META_SIZE);
    assert(_num_meta_data_bytes() == 7 * META_SIZE);

    // should enlarge wilderness block.
    b8 = smalloc(1000);
    assert(_num_free_blocks() == 0);
    assert(_num_free_bytes() == 0);
    assert(_num_allocated_blocks() == 7);
    assert(_num_allocated_bytes() == 21000);
    assert(_num_meta_data_bytes() == 7 * META_SIZE);

    // add another block to the end.
    b9 = smalloc(10);
    assert(_num_free_blocks() == 0);
    assert(_num_free_bytes() == 0);
    assert(_num_allocated_blocks() == 8);
    assert(_num_allocated_bytes() == 21012);
    assert(_num_meta_data_bytes() == 8 * META_SIZE);

    //
    free(b9);
    assert(_num_free_blocks() == 1);
    assert(_num_free_bytes() == 12);
    assert(_num_allocated_blocks() == 8);
    assert(_num_allocated_bytes() == 21012);
    assert(_num_meta_data_bytes() == 8 * META_SIZE);

    b9 = smalloc(8);
    assert(_num_free_blocks() == 0);
    assert(_num_free_bytes() == 0);
    assert(_num_allocated_blocks() == 8);
    assert(_num_allocated_bytes() == 21012);
    assert(_num_meta_data_bytes() == 8 * META_SIZE);

    // checking alignment
    b9 = smalloc(1);
    assert(_num_free_blocks() == 0);
    assert(_num_free_bytes() == 0);
    assert(_num_allocated_blocks() == 9);
    assert(_num_allocated_bytes() == 21016);
    assert(_num_meta_data_bytes() == 9 * META_SIZE);

    b9 = smalloc(2);
    assert(_num_free_blocks() == 0);
    assert(_num_free_bytes() == 0);
    assert(_num_allocated_blocks() == 10);
    assert(_num_allocated_bytes() == 21020);
    assert(_num_meta_data_bytes() == 10 * META_SIZE);

    b9 = smalloc(3);
    assert(_num_free_blocks() == 0);
    assert(_num_free_bytes() == 0);
    assert(_num_allocated_blocks() == 11);
    assert(_num_allocated_bytes() == 21024);
    assert(_num_meta_data_bytes() == 11 * META_SIZE);

    b9 = smalloc(4);
    assert(_num_free_blocks() == 0);
    assert(_num_free_bytes() == 0);
    assert(_num_allocated_blocks() == 12);
    assert(_num_allocated_bytes() == 21028);
    assert(_num_meta_data_bytes() == 12 * META_SIZE);

    return 0;
}
