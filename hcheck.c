#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <memory.h>
#include <stdbool.h>

#define rdtsc(x)      __asm__ __volatile__("rdtsc \n\t" : "=A" (*(x)))

#include "pa3.c"

#ifdef VHEAP
// for Part 2
    #define TESTSUITE_STR           "Virtualized Heap"
    #define INIT(msize)             VInit(msize)
    #define MALLOC(msize)           VMalloc(msize)
    #define FREE(addr, size)        VFree(addr)
    #define PUT(data,size)          VPut(data,size)
    #define GET(rt,addr,size)       VGet(rt,addr,size)
    #define ADDRS                   addrs_t*
    #define LOCATION_OF(addr)       ((size_t)(*addr))
    #define DATA_OF(addr)           (*(*(addr)))
    #define VALLOCK_BLK              valloc_blk
    #define VFREE_BLK               vfree_blk
    #define VRAW_ALLOC_BYTE         vraw_alloc_byte
    #define VPAD_ALLOC_BYTE         vpad_alloc_byte
    #define VRAW_FREE_BYTE          vraw_free_byte
    #define VALI_FREE_BYTE          vali_free_byte
    #define VMALLOC_NUM             vmalloc_num
    #define VFREE_NUM               vfree_num
    #define VFAIL_NUM               vfail_num

#else
// for Part 1
    #define TESTSUITE_STR           "Heap"
    #define INIT(msize)             Init(msize)
    #define MALLOC(msize)           Malloc(msize)
    #define FREE(addr,size)         Free(addr)
    #define PUT(data,size)          Put(data,size)
    #define GET(rt,addr,size)       Get(rt,addr,size)
    #define ADDRS                   addrs_t
    #define LOCATION_OF(addr)       ((size_t)addr)
    #define DATA_OF(addr)           (*(addr))
    #define ALLOC_BLK               alloc_blk
    #define FREE_BLK                free_blk
    #define RAW_ALLOC_BYTE          raw_alloc_byte
    #define PAD_ALLOC_BYTE          pad_alloc_byte
    #define RAW_FREE_BYTE           raw_free_byte
    #define ALI_FREE_BYTE           ali_free_byte
    #define MALLOC_NUM              malloc_num
    #define FREE_NUM                free_num
    #define FAIL_NUM                fail_num

#endif

// For Heap Checker to test time
int test_time(int numIterations, unsigned long* tot_alloc_time, unsigned long* tot_free_time){
    int i, n = 0;
    char s[80];
    #ifdef VHEAP
    addrs_t* addr;
    #else
    addrs_t addr;
    #endif
    char data[80];

    unsigned long start, finish;
    *tot_alloc_time = 0;
    *tot_free_time = 0;

    for (i = 0; i < numIterations; i++) {
        #ifdef VHEAP
        // Check put
        rdtsc(&start);
        addr = VPut(s, n+1);
        rdtsc(&finish);
        *tot_alloc_time += finish - start;
        // Check get
        rdtsc(&start);
        VGet((any_t)data, addr, n+1);
        rdtsc(&finish);
        *tot_free_time += finish - start;

        #else
        // Check put
        rdtsc(&start);
        addr = Put(s, n+1);
        rdtsc(&finish);
        *tot_alloc_time += finish - start;
        // Check get
        rdtsc(&start);
        Get((any_t)data, addr, n+1);
        rdtsc(&finish);
        *tot_free_time += finish - start;
        #endif
    }
}

int main(){
    unsigned mem_size = (1<<20);

    unsigned long tot_alloc_time, tot_free_time;
    int numIterations = 1000000;
    // Initialize the heap
    INIT(mem_size);

#ifdef VHEAP
    /* need to call functions here to run Part 2 */
    // Below is our own sample test function
    VInit(mem_size);
    addrs_t* v1 = VMalloc(8);
    addrs_t* v2 = VMalloc(4);
    VFree(v1);
    addrs_t* v3 = VMalloc(64);
    addrs_t* v4 = VMalloc(5);
    VFree(v4);
    VFree(v2);
    printf("Heap Checker for Part 2:\n");
    printf("Number of allocated blocks: %i\n", VALLOCK_BLK);
    printf("Number of free blocks: %i\n", VFREE_BLK);
    printf("Raw total number of bytes allocated: %i\n", VRAW_ALLOC_BYTE);
    printf("Padded total number of bytes allocated: %i\n", VPAD_ALLOC_BYTE);
    printf("Raw total number of bytes free: %i\n", VRAW_FREE_BYTE);
    printf("Aligned total number of bytes free: %i\n", VALI_FREE_BYTE);
    printf("Total number of VMalloc requests: %i\n", VMALLOC_NUM);
    printf("Total number of VFree requests: %i\n", VFREE_BLK);
    printf("Total number of request failures: %i\n", VFAIL_NUM);

    test_time(numIterations,&tot_alloc_time,&tot_free_time);
    printf("Average clock cycles for a VMalloc request: %lu\n", tot_alloc_time/numIterations);
    printf("Average clock cycles for a VFree request: %lu\n", tot_free_time/numIterations);
    printf("Total clock cycles for all requests: %lu\n", tot_alloc_time+tot_free_time);

#else
    /* need to call functions here to run Part 1 */
    // Below is our own sample test function
    Init(mem_size);
    addrs_t v1 = Malloc(8);
    addrs_t v2 = Malloc(4);
    Free(v1);
    addrs_t v3 = Malloc(64);
    addrs_t v4 = Malloc(5);
    Free(v4);
    Free(v2);
    printf("Heap Checker for Part 1:\n");
    printf("Number of allocated blocks: %i\n", ALLOC_BLK);
    printf("Number of free blocks: %i\n", FREE_BLK);
    printf("Raw total number of bytes allocated: %i\n", RAW_ALLOC_BYTE);
    printf("Padded total number of bytes allocated: %i\n", PAD_ALLOC_BYTE);
    printf("Raw total number of bytes free: %i\n", RAW_FREE_BYTE);
    printf("Aligned total number of bytes free: %i\n", ALI_FREE_BYTE);
    printf("Total number of Malloc requests: %i\n", MALLOC_NUM);
    printf("Total number of Free requests: %i\n", FREE_NUM);
    printf("Total number of request failures: %i\n", FAIL_NUM);

    test_time(numIterations,&tot_alloc_time,&tot_free_time);
    printf("\tAverage clock cycles for a Malloc request: %lu\n", tot_alloc_time/numIterations);
    printf("\tAverage clock cycles for a Free request: %lu\n", tot_free_time/numIterations);
    printf("\tTotal clock cycles for all requests: %lu\n", tot_alloc_time+tot_free_time);

#endif
    return 0;
}