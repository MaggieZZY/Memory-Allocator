
CS210 Final Project

Lijun Chen (U91123888)
Ziyao Zhang (U46618591)



Part 1:

The data structure we use in this project is linked list (to connect blocks of memory together). To do this, we use struct to define a new type Chunk. Chunk is a block that includes the following information: the pointer to the next block and the pointer to the previous block, the size of the block, whether it is already allocated or not and a data part that can be used to actually allocate memory. To initialize, we use the malloc() routine to allocate size byte of memory on M1. We set the address to its previous block, next block and whether it’s allocated all to 0. The size of it is the size of M1 subtracted by the memory to hold raw information.

Then we try to follow the general routine of the code provided in the lab section and the assignment handout. For Malloc function, we first test if the baseptr is 0 (whether the memory heap has generated or not). If it is 0, we call Init that takes in an extremely large value as parameter (we use 1<<30 in our code). Then, we adjust the size being allocated so that we can align all addresses on the 8 byte boundaries (to avoid alignment problems). After that we implement a first fit helper function and call the first fit function to keep searching until we find a block that is not allocated and also contains enough space to allocate the adjusted size memory. Next, we implement a place helper function to actually place the memory at the beginning of the free block and split it from the remaining.  

In the Free function, other than updating the information included in the block, we implement the idea of coalesce as indicated in lab’s code to merge the free blocks together depending on whether the previous and next block is allocated or not. But in our coalesce we only need to consider two cases which are merge with the previous and merge with the next. Put and Get functions are simple by using memcpy. 



Part 2:

For part 2, we create a new type called Redirect_T to represent the slot of the redirection table. It contains a pointer ptr pointing to an address in the memory heap, and a size, which is the size of the memory going to be allocated. To initialize, we use malloc() to allocate memory in the redirection table and the memory heap. The pointer of each slot of the redirection table is set to 0. mem_ptr can be seen as the base pointer of the memory heap. mem_size is the total size of the memory heap.

The structure of our VMalloc function function is similar to Part 1. We first test if the memory heap has generated or not. If it hasn’t, we call VInit that takes in an extremely large value as parameter (we use 1<<30 in our code). Then, we adjust the size of the memory to be allocated. To find the first fit, we not only go through the redirection table to search for a available slot with a pointer pointing to the first free block on the memory heap; we also go through the memory heap to find if there is sufficient space left for the memory to be allocated. If both conditions are satisfied, we place the memory to the memory heap and return the address.

In VFree, we first free the memory block with the address provided. The next step is kind of tricky since we actually need to create another redirection table of pointers pointing to the pointers in the old table. Then we use qsort to sort pointers in the new table into ascending order. Then we can go through the second redirection table to compact the space we just free. VPut and VGet are pretty much the same as in Part 1. 



To Test:

We put our part1 and part2 in a single file. However, we don’t exactly know how to enable/disable each part so we just follow the instructions on piazza provided by the professor.
Basically, our code looks like:
#ifndef VHEAP
 // Code for Part 1
#else
 // Code for Part 2
#endif


We put our heap checker in two places, in pa3.c (in the main function which is commented out) and in hcheck.c. Either of them works well. However, in order to make the heap checker return the expected output, you need to run the test in the main function of the file.


If you want to use hcheck.c, I modified the makefile in testsuite to implement it:
LIBS        = -lm
INC_PATH    = -I.
CC_FLAGS    = -g

all: pa31 pa32 check31 check32

pa31: testsuite.o pa3.o
	cc $(CC_FLAGS) $(INC_PATH) -o $@ testsuite.c $(LIBS)

pa32: testsuite.o pa3.o
	cc -DVHEAP $(CC_FLAGS) $(INC_PATH) -o $@ testsuite.c $(LIBS)

check31: hcheck.o pa3.o
	cc $(CC_FLAGS) $(INC_PATH) -o $@ hcheck.c $(LIBS)

check32: hcheck.o pa3.o
	cc -DVHEAP $(CC_FLAGS) $(INC_PATH) -o $@ hcheck.c $(LIBS)

clean :
	rm -f *.o *~ $(PROGS)