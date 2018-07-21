////////////////////////////////////////////////////////////////////////////////
// Main File:        mem.c
// This File:        mem.c
// Semester:         CS 354 Fall 2017
//
// Author:           William Crouch
// Email:            wcrouch@wisc.edu
// CS Login:         crouch@cs.wisc.edu
//
////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include "mem.h"

/*
 * This structure serves as the header for each allocated and free block
 * It also serves as the footer for each free block
 * The blocks are ordered in the increasing order of addresses 
 */
typedef struct blk_hdr {   	    

    int size_status;
  
    /*
    * Size of the block is always a multiple of 8
    * => last two bits are always zero - can be used to store other information
    *
    * LSB -> Least Significant Bit (Last Bit)
    * SLB -> Second Last Bit 
    * LSB = 0 => free block
    * LSB = 1 => allocated/busy block
    * SLB = 0 => previous block is free
    * SLB = 1 => previous block is allocated/busy
    * 
    * When used as the footer the last two bits should be zero
    */

    /*
    * Examples:
    * 
    * For a busy block with a payload of 20 bytes (i.e. 20 bytes data + an additional 4 bytes for header)
    * Header:
    * If the previous block is allocated, size_status should be set to 27
    * If the previous block is free, size_status should be set to 25
    * 
    * For a free block of size 24 bytes (including 4 bytes for header + 4 bytes for footer)
    * Header:
    * If the previous block is allocated, size_status should be set to 26
    * If the previous block is free, size_status should be set to 24
    * Footer:
    * size_status should be 24
    * 
    */

} blk_hdr;

/* Global variable - This will always point to the first block
 * i.e. the block with the lowest address */
blk_hdr *first_block = NULL;

// Global variable - Total available memory 
int total_mem_size = 0;

/*
 * This function is a helper method used to get the next block
 * Returns the pointer to the next block header
 */
blk_hdr* getNext(blk_hdr* current){
    
    blk_hdr *next = NULL;
    blk_hdr *max = (void*)first_block + (total_mem_size) + 2;
    
    //if currPos is NULL, there is no next block
    if(current == NULL){
        return NULL;
    }
    
    next = (void*)current + (current->size_status -(current-> size_status % 4));
    
    //if next is NULL, currPos was the last block in the chain
    if(next == NULL){
        return NULL;
    }
    //if currPoss was invalid, there is no next
    if(current->size_status <= 0 || (current->size_status > total_mem_size)){
        return NULL;
    }
    
    //if next is invalid, there is no next.
    if(next >= max){
        return NULL;		
    }
    return next;
    
}


/* 
 * Function for allocating 'size' bytes
 * Returns address of allocated block on success 
 * Returns NULL on failure 
 * Here is what this function should accomplish 
 * - Check for sanity of size - Return NULL when appropriate 
 * - Round up size to a multiple of 8 
 * - Traverse the list of blocks and allocate the best free block which can accommodate the requested size 
 * - Also, when allocating a block - split it into two blocks
 * Tips: Be careful with pointer arithmetic 
 */
void* Mem_Alloc(int size)
{
    //check for sanity
    if(size < 0){
        return NULL;
    }
    int headerSize = 4;
    size = size + headerSize;
    if(size > total_mem_size)
    {
        return NULL;
    }
    //round up size to make multiple of 8
    int mod = size % 8;
    if(mod != 0){
        size = size + (8 - mod);
    }
    blk_hdr *current = first_block;
    blk_hdr *previous = NULL;
    blk_hdr *best = NULL;
    blk_hdr *oldBest = NULL;

    //search through list to find a bit
    while(current != NULL){
        if((current->size_status & 1) == 0){
            if(current->size_status >= size){
                //first case
                if(best == NULL){
                    oldBest = previous;
                    best = current;
                   
                }
                
                //best case
                else if(current->size_status == size){
                    best = current;
                    oldBest = previous;
                    break;
                    
                    
                }
                //finds a new better case
                else if((current->size_status - size) < (best->size_status -size)){
                    oldBest = previous;
                    best = current;
                    
                }
            }		
        }
        
        previous = current;
        current = getNext(current);
        
    }
    
    //Now we have to allocate it
    if(best != NULL)
    {
        if(best->size_status - size >= 8 ){ //8 is minimum size for double word alignment
            blk_hdr *newblock = (void*)best + size;
            newblock->size_status = (best->size_status - (best->size_status %4)) - size + 2;
            best->size_status = size;
        }
        //check if previous block is allocated
        
        if((oldBest == NULL || ((oldBest->size_status & 1) != 0)) && ((best->size_status & 2) == 0)){
            best->size_status += 2;
        }
        //mark block as allocated
        best->size_status += 1;
        return (void*)best + headerSize;
    }
    else
    {
        return NULL;
    }


    
    
}

/*
 * Function for freeing up a previously allocated block 
 * Argument - ptr: Address of the block to be freed up 
 * Returns 0 on success 
 * Returns -1 on failure 
 * Here is what this function should accomplish 
 * - Return -1 if ptr is NULL or not within the range of memory allocated by Mem_Init()
 * - Return -1 if ptr is not 8 byte aligned or if the block is already freed
 * - Mark the block as free 
 * - Coalesce if one or both of the immediate neighbours are free 
 */
int Mem_Free(void *ptr) {   	   
    // Your code goes in here
    int headerSize = 4;
    if(ptr == NULL)
    {
        return -1;
    }
    int works = 0;
    blk_hdr *search = first_block;
    ptr = (void*)ptr - headerSize;
    while(search != NULL){
        if((void*)search == ptr){
            works = 1;
            break;
        }
        search = getNext(search);
    }	

    
    //If the pointer works
    if(works){
        blk_hdr *curr  = (blk_hdr*)ptr;
        curr -> size_status = curr -> size_status - 1; //free the block
        blk_hdr *prev = first_block; //going to search through
        blk_hdr *next = getNext(curr);
        if(curr == first_block){
            prev = NULL;
        }
        else
        {
            //search through list to find the previous block
            while(getNext(prev) != curr){
                prev = getNext(prev);
            }
            
        }
        //Booleans used to keep track of status of next and previous blocks
        int prevI = 0;
        int nextI = 0;
        
        //nothing to do here
        if(prev == NULL && next == NULL){
            return 0;
        }
        
        //if there is a not a next block, but is a prev
        else if(prev != NULL && next == NULL){
            //left was allocated
            if((prev->size_status & 1) != 0){
                return 0;
                
            }
            //	left is free! coalesce it
            else if((prev->size_status & 1) == 0){
                prevI = 1;
                
            }
            
        }

        //if there is not a prev block, but is a next block
        else if(prev == NULL && next != NULL){
            //right was allocated
            if((next->size_status & 1) != 0){
                return 0;
            }
            //right was free! coalesce it
            else if((next->size_status & 1) == 0){
                nextI = 1;
            }
            
        }
        
        //nothing to coalese in this case
        else if (((prev->size_status & 1) != 0) && ((next->size_status & 1) != 0))
        {
            return 0;
        }
        //previous is free
        else if (((prev->size_status & 1) != 0) && ((next->size_status & 1) == 0))
        {
            nextI = 1;
            
        }
        //next is free
        else if (((prev->size_status & 1) == 0) && ((next->size_status & 1) != 0))
        {
            prevI = 1;
           
        }
        //both are free
        else if (((prev->size_status & 1) == 0) && ((next->size_status & 1) == 0)) {
            prevI = 1;
            nextI = 1;
        }
        //take the size_statuses and remove the status bits
        int currentSize = (curr->size_status) - (curr->size_status%4);
        int nextSize = (next -> size_status) - (next->size_status%4);
        int prevSize = 0;
        if(prev != NULL)
        {
            prevSize = (prev ->size_status) - (prev->size_status%4);
        }
        
        //if both need coalescing
        if (prevI ==  1 && nextI == 1)
        {
            int size = 0;
            if((prev->size_status & 2) == 2)
            {
                size = 2;
            }
            
            prev->size_status = prevSize + currentSize + nextSize + size;
            blk_hdr *nfooter = (void*)curr + currentSize - headerSize;
            if ((void *)nfooter == (void *)first_block + total_mem_size) {
                return 0;
            }
            nfooter->size_status = (prev->size_status) - (prev->size_status%4);
            
        
        }
        //if only right needs coalescing
        else if (prevI == 0 && nextI == 1)
        {
            int size = 0;
            if((curr->size_status & 2) == 2){
                size = 2;
            }
            curr->size_status = currentSize + nextSize + size;
            blk_hdr *nfoot2 = (void*)next + nextSize - headerSize;
            if ((void *)nfoot2 == (void *)first_block + total_mem_size - headerSize) {
                return 0;
            }
            
            nfoot2->size_status = (curr->size_status) -(curr->size_status%4);
        }
        //if only left needs coalescing
        else if (prevI == 1 && nextI == 0)
        {
            int size = 0;
            if((prev->size_status & 2) == 2){
                size = 2;
            }
            prev->size_status = prevSize + currentSize + size;
            blk_hdr *cfoot = (void*)curr + currentSize - headerSize;
            cfoot -> size_status = (prev-> size_status) - (prev->size_status%4);
        }
        
        //make sure the next block SLSB is set to 0
        blk_hdr *fixNext = getNext(curr);
        if(fixNext != NULL && (fixNext->size_status & 2) == 2)
        {
            fixNext->size_status = fixNext->size_status - 2;
        }
        return 0;
        
    }
    else
    {
        return -1;
    }
    
    

}

/*
 * For testing purpose
 * To verify whether a block is double word aligned
 */
void *start_pointer;

/*
 * Function used to initialize the memory allocator
 * Not intended to be called more than once by a program
 * Argument - sizeOfRegion: Specifies the size of the chunk which needs to be allocated
 * Returns 0 on success and -1 on failure 
 */
int Mem_Init(int sizeOfRegion) {   	   
    int pagesize;
    int padsize;
    int fd;
    int alloc_size;
    void* space_ptr;
    static int allocated_once = 0;
  
    if(0 != allocated_once) {
        fprintf(stderr,"Error:mem.c: Mem_Init has allocated space during a previous call\n");
        return -1;
    }
    if(sizeOfRegion <= 0) {
        fprintf(stderr,"Error:mem.c: Requested block size is not positive\n");
        return -1;
    }

    // Get the pagesize
    pagesize = getpagesize();

    // Calculate padsize as the padding required to round up sizeOfRegion to a multiple of pagesize
    padsize = sizeOfRegion % pagesize;
    padsize = (pagesize - padsize) % pagesize;

    alloc_size = sizeOfRegion + padsize;

    // Using mmap to allocate memory
    fd = open("/dev/zero", O_RDWR);
    if(-1 == fd){
        fprintf(stderr,"Error:mem.c: Cannot open /dev/zero\n");
        return -1;
    }
    space_ptr = mmap(NULL, alloc_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (MAP_FAILED == space_ptr) {
        fprintf(stderr,"Error:mem.c: mmap cannot allocate space\n");
        allocated_once = 0;
        return -1;
    }
  
    allocated_once = 1;
 
    // for double word alignement
    alloc_size -= 4;
 
    // Intialising total available memory size
    total_mem_size = alloc_size;

    // To begin with there is only one big free block
    // initialize heap so that first block meets double word alignement requirement
    first_block = (blk_hdr*) space_ptr + 1;
    start_pointer = space_ptr;
  
    // Setting up the header
    first_block->size_status = alloc_size;

    // Marking the previous block as busy
    first_block->size_status += 2;

    // Setting up the footer
    blk_hdr *footer = (blk_hdr*) ((char*)first_block + alloc_size - 4);
    footer->size_status = alloc_size;
  
    return 0;
}

/* 
 * Function to be used for debugging 
 * Prints out a list of all the blocks along with the following information for each block 
 * No.      : serial number of the block 
 * Status   : free/busy 
 * Prev     : status of previous block free/busy
 * t_Begin  : address of the first byte in the block (this is where the header starts) 
 * t_End    : address of the last byte in the block 
 * t_Size   : size of the block (as stored in the block header)(including the header/footer)
 */ 
void Mem_Dump() {   	   
    int counter;
    char status[5];
    char p_status[5];
    char *t_begin = NULL;
    char *t_end = NULL;
    int t_size;

    blk_hdr *current = first_block;
    counter = 1;

    int busy_size = 0;
    int free_size = 0;
    int is_busy = -1;

    fprintf(stdout,"************************************Block list***********************************\n");
    fprintf(stdout,"No.\tStatus\tPrev\tt_Begin\t\tt_End\t\tt_Size\n");
    fprintf(stdout,"---------------------------------------------------------------------------------\n");
  
    while (current < (blk_hdr*) ((char*)first_block + total_mem_size)) {
        t_begin = (char*)current;
        t_size = current->size_status;
    
        if (t_size & 1) {
            // LSB = 1 => busy block
            strcpy(status,"Busy");
            is_busy = 1;
            t_size = t_size - 1;
        }
        else {
            strcpy(status,"Free");
            is_busy = 0;
        }

        if (t_size & 2) {
            strcpy(p_status,"Busy");
            t_size = t_size - 2;
        }
        else 
            strcpy(p_status,"Free");

        if (is_busy) 
            busy_size += t_size;
        else 
            free_size += t_size;

        t_end = t_begin + t_size - 1;
    
        fprintf(stdout,"%d\t%s\t%s\t0x%08lx\t0x%08lx\t%d\n", counter, status, p_status, 
                    (unsigned long int)t_begin, (unsigned long int)t_end, t_size);
    
        current = (blk_hdr*)((char*)current + t_size);
        counter = counter + 1;
    }

    fprintf(stdout,"---------------------------------------------------------------------------------\n");
    fprintf(stdout,"*********************************************************************************\n");
    fprintf(stdout,"Total busy size = %d\n", busy_size);
    fprintf(stdout,"Total free size = %d\n", free_size);
    fprintf(stdout,"Total size = %d\n", busy_size + free_size);
    fprintf(stdout,"*********************************************************************************\n");
    fflush(stdout);

    return;
}
