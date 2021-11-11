#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <stdlib.h>
#include <stdbool.h>

typedef char *addrs_t;
typedef void *any_t;

#define SIZE(T, E) ((size_t)&(((T *)0)->E))
#define rdtsc(x)      __asm__ __volatile__("rdtsc \n\t" : "=A" (*(x)))

/*
 * Part 1
 */

#ifndef VHEAP

/* creat a link list */
typedef struct Chunk{
    struct Chunk *prev, *next;
    size_t size, allocated;
    char data[1];
} Chunk;

/* initial pointer value */
struct Chunk *baseptr = 0;

/* Function prototypes for internal helper routines */
static void coalesce (Chunk * bp);
static void place (Chunk * bp, size_t asize);
static Chunk * find_first_fit (size_t asize);

/* for Heap Checker */
int alloc_blk = 0;
int free_blk = 0; //discounting padding bytes
int raw_alloc_byte = 0; //the actual total bytes requested
int pad_alloc_byte = 0; //the total bytes requested plus internally fragmented blocks wasted due to padding/alignment
int raw_free_byte = 0;
int ali_free_byte = 0; //sizeof(M1) - pad_alloc_byte (account for meta-datastructures inside M1 also)
int malloc_num = 0;
int free_num = 0;
int fail_num = 0; //unable to satisfy the allocation or de-allocation requests
int M1 = 0;

/* 
 * init - Initialize the memory manager
 *        Use the system malloc() routine (or new in C++) only to allocate
 *        size bytes for the initial memory area, M1.
 *        baseptr is the starting address of M1.
 */
void Init (size_t size) {
    if(size < SIZE(Chunk,data))
        return;

    baseptr = (Chunk*) malloc (size);
    baseptr->prev = baseptr->next = 0;
    baseptr->allocated = 0;
    baseptr->size = size - SIZE(Chunk,data);

    raw_free_byte = M1 = size;
    ali_free_byte = baseptr->size;
}

/* 
 * Malloc - Implement your own memory allocation routine here. 
 *          This should allocate the first contiguous size bytes available in M1. 
 *          It is safe to allocate space starting at the first address divisible by 8.
 *          Hence align all addresses on 8-byte boundaries!
 *          If enough space exists, allocate space and return the base address.
 *          If insufficient space exists, return NULL. 
 */
addrs_t Malloc (size_t size) {
    malloc_num ++;

    if (baseptr == 0){
        Init(1<<30);
    }

    size_t asize;
    if (size%8 == 0){
        asize = size;
    }
    else{
        asize = (size/8 + 1) * 8;
    }

    Chunk *current = find_first_fit(asize);
    if(current == 0){
        fail_num ++;
        return 0;
    }
    else{
        place(current, asize);
        alloc_blk ++;
        raw_alloc_byte += size;
        pad_alloc_byte += asize;
        raw_free_byte -= size;
        ali_free_byte -= asize; //SIZE(Chunk, data) - pad_alloc_byte;
        return current->data;
    }
}

/* 
 * Malloc helper: find_first_fit - Find a fit for a block with asize bytes 
 */
static Chunk *find_first_fit (size_t asize){
    Chunk *current = baseptr;
    for (current = baseptr; current != 0;current = current->next){
        if(!current->allocated && current->size >= asize){
            return current;
        }
    }
}

/* 
 * Malloc helper: place - Place block of asize bytes at start of free block bp
 *                and split if remainder would be at least minimum block size
 */
 
static void place (Chunk * current, size_t asize){
    size_t left = current->size - asize;
    if (left >= SIZE(Chunk,data)){
        Chunk * available = (Chunk*)(current->data + asize);
        available->allocated = false;
        available->prev = current;
        available->next = current->next;
        available->size = current->size - asize - SIZE(Chunk,data);

        if (current->next){
            current->next->prev = available;
        }
        current->size = asize;
        current->next = available;
    }
    current->allocated = true;
}

/* 
 * Free - This frees the previously allocated size bytes starting from address addr
 *        in the memory area, M1. You can assume the size argument is stored in
 *        a data structure after the Malloc() routine has been called, just as
 *        with the UNIX free() command. 
 */
void Free (addrs_t addr) {
    free_num ++;

    if (addr == 0){
        fail_num ++;
        return;
    }
    if (baseptr == 0){
        fail_num ++;
        return;
    }

    Chunk* node = (Chunk*)(addr - SIZE(Chunk,data));
    node->allocated = false;

    free_blk ++;
    alloc_blk --;
    raw_free_byte = M1 - raw_alloc_byte;
    ali_free_byte = M1 - pad_alloc_byte;

    coalesce(node);
}

/* 
 * Free helper: coalesce - Boundary tag coalescing. Return ptr to coalesced block
 */
static void coalesce (Chunk * node){
    // Merge before
    if(node->prev && !node->prev->allocated){
        Chunk *prev = node->prev;
        prev->size += SIZE(Chunk,data) + node->size;
        prev->next = node->next;
        if(node->next){
            node->next->prev = prev;
        }
        node = prev;
    }
    // Merge after
    if(node->next && !node->next->allocated){
        Chunk*next = node->next;
        node->size += SIZE(Chunk,data) + next->size;
        node->next = next->next;
        if(next->next){
            next->next->prev = node;
        }
    }
}

/*
 * Put - Allocate size bytes from M1 using Malloc(). 
 *       Copy size bytes of data into Malloc'd memory. 
 *       You can assume data is a storage area outside M1. 
 *       Return starting address of data in Malloc'd memory.
 */
addrs_t Put (any_t data, size_t size) {
    addrs_t sa = Malloc(size);
    if (sa != NULL){
        memcpy(sa,data,size);
    }
    return sa;
}

/* 
 * Get - Copy size bytes from addr in the memory area, M1, to data address. 
 *       As with Put(), you can assume data is a storage area outside M1. 
 *       De-allocate size bytes of memory starting from addr using Free().
 */
void Get (any_t return_data, addrs_t addr, size_t size) {
    memcpy(return_data,addr,size);
    Free(addr);
}

/*
 * Part 2
 */
#else

#define MAX(x, y) ((x) > (y)? (x) : (y))

/* create a redirection table */
typedef struct Redirect_T{
    addrs_t ptr;
    size_t size;
} Redirect_T;

/* initiate redirection table pointer */
Redirect_T *rtable = 0;

/* Function prototypes for internal helper routines */
int compare(const void * x, const void * y);

const size_t R = 100;
addrs_t mem_ptr = 0;
size_t mem_size = 0;

/* for Heap Checker */
int valloc_blk = 0;
int vfree_blk = 0; //discounting padding bytes
int vraw_alloc_byte = 0; //the actual total bytes requested
int vpad_alloc_byte = 0; //the total bytes requested plus internally fragmented blocks wasted due to padding/alignment
int vraw_free_byte = 0;
int vali_free_byte = 0; //sizeof(M1) - pad_alloc_byte (account for meta-datastructures inside M1 also)
int vmalloc_num = 0;
int vfree_num = 0;
int vfail_num = 0; //unable to satisfy the allocation or de-allocation requests
int M2 = 0;


/* 
 * init - Initialize redirection table and memory area
 */
void VInit(size_t size){
    if (size < sizeof(Redirect_T)){
        return;
    }

    /* get first pointer to the redirection table */
    rtable = (Redirect_T*)malloc(sizeof(Redirect_T)*R);

    /* generate empty redirection table */
    int i;
    for(i = 0; i < R; i++){
        rtable[i].ptr=0;
    }

    /* get first pointer to memory area and set the size of the memory area */
    mem_ptr = (addrs_t)malloc(size);
    mem_size = size;

    vraw_free_byte = vali_free_byte = M2 = size;
}

addrs_t *VMalloc (size_t size){
    vmalloc_num ++;

    if (rtable == 0 || mem_ptr == 0){
        VInit(1<<21);
    }

    /* size adjustment for alignment requirement */
    size_t asize;
    if (size%8 == 0){
        asize = size;
    }
    else{
        asize = (size/8 + 1) * 8;
    }

    /* find first avaliable place in redirectiont table & get the number */
    size_t index;
    for(index = 0; index < R; index++){
        if(rtable[index].ptr == 0){
            break;
        }
    }
    if (index == R-1 && rtable[index].ptr != 0){
        vfail_num ++;
        return 0;
    }

    /* find next free memory pointer and test if there is sufficient space */
    addrs_t last = mem_ptr;
    size_t i;
    for(i = 0; i < R; i++){
        if(rtable[i].ptr != 0){
            last = MAX(last, rtable[i].ptr + rtable[i].size);
        }
    }
    if(last + asize > mem_ptr + mem_size){
        vfail_num ++;
        return 0;
    }

    /* link redirection table and memory area with input */
    rtable[index].ptr = last;
    rtable[index].size = asize;

    valloc_blk ++;
    vraw_alloc_byte += size;
    vpad_alloc_byte += asize;
    vraw_free_byte -= size;
    vali_free_byte -= asize;

    return &(rtable[index].ptr);
}

void VFree (addrs_t *addr){
    vfree_num ++;

    if (*addr == 0){
        vfail_num ++;
        return;
    }
    if (rtable == 0 || mem_ptr == 0){
        vfail_num ++;
        return;
    }

    /* Free the the memory with addr */
    Redirect_T *victim = (Redirect_T*)(addr - SIZE(Redirect_T, ptr));
    victim->ptr = 0;

    vfree_blk ++;
    valloc_blk --;
    vraw_free_byte = M2 - vraw_alloc_byte;
    vali_free_byte = M2 - vpad_alloc_byte;

    /* create a new table of pointer each point to the old table pointer */
    Redirect_T *ptable[R];
    size_t i;
    for(i = 0; i < R; i++){
        ptable[i] = rtable + i;
    }

    /* sort ptable in acending order */
    qsort(ptable, R, sizeof(addrs_t), compare);

    // Compaction
    addrs_t lower = mem_ptr;
    size_t m;
    for(m = 0; m < R; m++){
        if(ptable[m]->ptr != 0){
            memcpy(lower, ptable[m]->ptr, ptable[m]->size);
            ptable[m]->ptr = lower;
            lower += ptable[m]->size;
        }
    }
}

int compare(const void * x, const void * y){
    Redirect_T *a = (Redirect_T*)x;
    Redirect_T *b = (Redirect_T*)y;
    if (a->ptr < b->ptr){
        return -1;
    }
    else {
        return 1;
    }
}

addrs_t *VPut (any_t data, size_t size) {
    addrs_t *ret = VMalloc(size);
    if (ret != 0){
        memcpy(*ret,data,size);
    }
    return ret;
}

void VGet (any_t return_data, addrs_t*addr, size_t size) {
    memcpy(return_data,*addr,size);
}
#endif

// For Heap Checker to test time
// int test_time(int numIterations, unsigned long* tot_alloc_time, unsigned long* tot_free_time){
//     int i, n = 0;
//     char s[80];
//     #ifdef VHEAP
//     addrs_t* addr;
//     #else
//     addrs_t addr;
//     #endif
//     char data[80];

//     unsigned long start, finish;
//     *tot_alloc_time = 0;
//     *tot_free_time = 0;

//     for (i = 0; i < numIterations; i++) {
//         #ifdef VHEAP
//         // Check put
//         rdtsc(&start);
//         addr = VPut(s, n+1);
//         rdtsc(&finish);
//         *tot_alloc_time += finish - start;
//         // Check get
//         rdtsc(&start);
//         VGet((any_t)data, addr, n+1);
//         rdtsc(&finish);
//         *tot_free_time += finish - start;

//         #else
//         // Check put
//         rdtsc(&start);
//         addr = Put(s, n+1);
//         rdtsc(&finish);
//         *tot_alloc_time += finish - start;
//         // Check get
//         rdtsc(&start);
//         Get((any_t)data, addr, n+1);
//         rdtsc(&finish);
//         *tot_free_time += finish - start;
//         #endif
//     }
// }
//
// int main(){
//     unsigned mem_size = (1<<20);

//     unsigned long tot_alloc_time, tot_free_time;
//     int numIterations = 1000000;

// #ifdef VHEAP
//     /* need to call functions here to run Part 2 */
//     // Below is our own sample test function
//     VInit(mem_size);
//     addrs_t* v1 = VMalloc(8);
//     addrs_t* v2 = VMalloc(4);
//     VFree(v1);
//     addrs_t* v3 = VMalloc(64);
//     addrs_t* v4 = VMalloc(5);
//     VFree(v4);
//     VFree(v2);

//     printf("Heap Checker for Part 2:\n");
//     printf("Number of allocated blocks: %i\n", valloc_blk);
//     printf("Number of free blocks: %i\n", vfree_blk);
//     printf("Raw total number of bytes allocated: %i\n", vraw_alloc_byte);
//     printf("Padded total number of bytes allocated: %i\n", vpad_alloc_byte);
//     printf("Raw total number of bytes free: %i\n", vraw_free_byte);
//     printf("Aligned total number of bytes free: %i\n", vali_free_byte);
//     printf("Total number of VMalloc requests: %i\n", vmalloc_num);
//     printf("Total number of VFree requests: %i\n", vfree_num);
//     printf("Total number of request failures: %i\n", vfail_num);
//     test_time(numIterations,&tot_alloc_time,&tot_free_time);
//     printf("Average clock cycles for a VMalloc request: %lu\n", tot_alloc_time/numIterations);
//     printf("Average clock cycles for a VFree request: %lu\n", tot_free_time/numIterations);
//     printf("Total clock cycles for all requests: %lu\n", tot_alloc_time+tot_free_time);

// #else
//     /* need to call functions here to run Part 1 */
//     // Below is our own sample test function
//     Init(mem_size);
//     addrs_t v1 = Malloc(8);
//     addrs_t v2 = Malloc(4);
//     Free(v1);
//     addrs_t v3 = Malloc(64);
//     addrs_t v4 = Malloc(5);
//     Free(v4);
//     Free(v2);
//     printf("Heap Checker for Part 1:\n");
//     printf("Number of allocated blocks: %i\n", alloc_blk);
//     printf("Number of free blocks: %i\n", free_blk);
//     printf("Raw total number of bytes allocated: %i\n", raw_alloc_byte);
//     printf("Padded total number of bytes allocated: %i\n", pad_alloc_byte);
//     printf("Raw total number of bytes free: %i\n", raw_free_byte);
//     printf("Aligned total number of bytes free: %i\n", ali_free_byte);
//     printf("Total number of Malloc requests: %i\n", malloc_num);
//     printf("Total number of Free requests: %i\n", free_num);
//     printf("Total number of request failures: %i\n", fail_num);
//     test_time(numIterations,&tot_alloc_time,&tot_free_time);
//     printf("Average clock cycles for a Malloc request: %lu\n", tot_alloc_time/numIterations);
//     printf("Average clock cycles for a Free request: %lu\n", tot_free_time/numIterations);
//     printf("Total clock cycles for all requests: %lu\n", tot_alloc_time+tot_free_time);

// #endif
//     return 0;
// }