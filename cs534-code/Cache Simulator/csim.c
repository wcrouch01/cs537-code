/* Name: William Crouch
 * CS login: crouch
 * Section(s): Lec 1
 *
 * csim.c - A cache simulator that can replay traces from Valgrind
 *     and output statistics such as number of hits, misses, and
 *     evictions.  The replacement policy is LRU.
 *
 * Implementation and assumptions:
 *  1. Each load/store can cause at most one cache miss plus a possible eviction.
 *  2. Instruction loads (I) are ignored.
 *  3. Data modify (M) is treated as a load followed by a store to the same
 *  address. Hence, an M operation can result in two cache hits, or a miss and a
 *  hit plus a possible eviction.
 *
 * The function printSummary() is given to print output.
 * Please use this function to print the number of hits, misses and evictions.
 * This is crucial for the driver to evaluate your work. 
 */

#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

/****************************************************************************/
/***** DO NOT MODIFY THESE VARIABLE NAMES ***********************************/

/* Globals set by command line args */
int verbosity = 0; /* print trace if set */
int s = 0; /* set index bits */
int b = 0; /* block offset bits */
int E = 0; /* associativity */
char* trace_file = NULL;

/* Derived from command line args */
int S; /* number of sets S = 2^s In C, you can use the left shift operator */
int B; /* block size (bytes) B = 2^b */

/* Counters used to record cache statistics */
int miss_count = 0;
int hit_count = 0;
int eviction_count = 0;
/*****************************************************************************/


/* Type: Memory address 
 * Use this type whenever dealing with addresses or address masks
 */
typedef unsigned long long int mem_addr_t;

/* Type: Cache line
 * TODO 
 * 
 * NOTE: 
 * You might (not necessarily though) want to add an extra field to this struct
 * depending on your implementation
 * 
 * For example, to use a linked list based LRU,
 * you might want to have a field "struct cache_line * next" in the struct 
 */
typedef struct cache_line {
    char valid;
    mem_addr_t tag;
    struct cache_line * next;
} cache_line_t;

typedef cache_line_t* cache_set_t;
typedef cache_set_t* cache_t;


/* The cache we are simulating */
cache_t cache;  

/* TODO - COMPLETE THIS FUNCTION
 * initCache - 
 * Allocate data structures to hold info regrading the sets and cache lines
 * use struct "cache_line_t" here
 * Initialize valid and tag field with 0s.
 * use S (= 2^s) and E while allocating the data structures here
 */
void initCache()
{
    //calc S
    S = (int) pow(2, s);
    cache = (cache_t) malloc(S * sizeof(cache_set_t));
    cache_set_t back_pointer = NULL;
    //Loop through set
    for(int i = 0; i < S; i++)
    {
        cache_set_t current_pointer = NULL;
        //Loop through lines
        for(int k = 0; k < E; k++)
        {
            cache_set_t line = (cache_set_t) malloc(sizeof(cache_line_t));
            line->next = NULL;
            line->valid = 0;
            line->tag = 0;
            
            if(current_pointer == NULL)
            {
                current_pointer = line;
                back_pointer = line;
                continue;
            }
            back_pointer->next = line;
            back_pointer = line;
            //back pointer is to fully link lines
        }
        //add to cash
        *(cache + i) = current_pointer;
    }
    
}


/* TODO - COMPLETE THIS FUNCTION 
 * freeCache - free each piece of memory you allocated using malloc 
 * inside initCache() function
 */
void freeCache()
{
    cache_set_t ctr = NULL;
    cache_set_t nctr = NULL;
    //loop through sets
    for(int i = 0; i < S; i++)
    {
        ctr = *(i + cache);
        //loop through lines
        while(ctr != NULL)
        {
            nctr = ctr->next;
            free(ctr);
            ctr = nctr;
        }
    }
    free(cache);
    //free pointer
    cache = NULL;
}

/* TODO - COMPLETE THIS FUNCTION
 * accessData - Access data at memory address addr.
 *   If it is already in cache, increase hit_count
 *   If it is not in cache, bring it in cache, increase miss count.
 *   Also increase eviction_count if a line is evicted.
 *   you will manipulate data structures allocated in initCache() here
 */
void accessData(mem_addr_t addr)
{
    mem_addr_t lineT = addr >> (s + b);
    unsigned int bitsize = (addr >> b) & (S - 1);
    //get current and previous pointers
    cache_set_t ctr = *(bitsize + cache);
    cache_set_t ptr = *(bitsize + cache);
    for(int i = 0; i < E; i++)
    {
        //check line
        if(ctr->valid == 1 && ctr->tag == lineT)
        {
            //mark a hit
            hit_count = hit_count + 1;
            if(ptr != ctr)
            {
                ptr->next = ctr->next;
                ctr->next = *(cache + bitsize);
            }
            *(cache + bitsize) = ctr;
            return;
        }
        if((E - 1) != i)
        {
            ptr = ctr;
            ctr = ctr->next;
        }
    }
    //otherwise it missed
    miss_count = miss_count + 1;
    if(ctr->valid == 1)
    {
        eviction_count++;
    }
    ctr->tag = lineT;
    ctr->valid = 1;
    
    if(ctr != ptr)
    {
        ptr->next = ctr->next;
        ctr->next = *(cache + bitsize);
    }
    *(cache + bitsize) = ctr;
    
}

/* TODO - FILL IN THE MISSING CODE
 * replayTrace - replays the given trace file against the cache 
 * reads the input trace file line by line
 * extracts the type of each memory access : L/S/M
 * YOU MUST TRANSLATE one "L" as a load i.e. 1 memory access
 * YOU MUST TRANSLATE one "S" as a store i.e. 1 memory access
 * YOU MUST TRANSLATE one "M" as a load followed by a store i.e. 2 memory accesses 
 */
void replayTrace(char* trace_fn)
{
    char buf[1000];
    mem_addr_t addr=0;
    unsigned int len=0;
    FILE* trace_fp = fopen(trace_fn, "r");

    if(!trace_fp){
        fprintf(stderr, "%s: %s\n", trace_fn, strerror(errno));
        exit(1);
    }

    while( fgets(buf, 1000, trace_fp) != NULL) {
        if(buf[1]=='S' || buf[1]=='L' || buf[1]=='M') {
            sscanf(buf+3, "%llx,%u", &addr, &len);
      
            if(verbosity)
                printf("%c %llx,%u ", buf[1], addr, len);
            
            accessData(addr);
            
            if(buf[1] == 'M') {
                accessData(addr);
            }
           // TODO - MISSING CODE
           // now you have: 
           // 1. address accessed in variable - addr 
           // 2. type of acccess(S/L/M)  in variable - buf[1] 
           // call accessData function here depending on type of access

            if (verbosity)
                printf("\n");
        }
    }

    fclose(trace_fp);
}

/*
 * printUsage - Print usage info
 */
void printUsage(char* argv[])
{
    printf("Usage: %s [-hv] -s <num> -E <num> -b <num> -t <file>\n", argv[0]);
    printf("Options:\n");
    printf("  -h         Print this help message.\n");
    printf("  -v         Optional verbose flag.\n");
    printf("  -s <num>   Number of set index bits.\n");
    printf("  -E <num>   Number of lines per set.\n");
    printf("  -b <num>   Number of block offset bits.\n");
    printf("  -t <file>  Trace file.\n");
    printf("\nExamples:\n");
    printf("  linux>  %s -s 4 -E 1 -b 4 -t traces/yi.trace\n", argv[0]);
    printf("  linux>  %s -v -s 8 -E 2 -b 4 -t traces/yi.trace\n", argv[0]);
    exit(0);
}

/*
 * printSummary - Summarize the cache simulation statistics. Student cache simulators
 *                must call this function in order to be properly autograded.
 */
void printSummary(int hits, int misses, int evictions)
{
    printf("hits:%d misses:%d evictions:%d\n", hits, misses, evictions);
    FILE* output_fp = fopen(".csim_results", "w");
    assert(output_fp);
    fprintf(output_fp, "%d %d %d\n", hits, misses, evictions);
    fclose(output_fp);
}

/*
 * main - Main routine 
 */
int main(int argc, char* argv[])
{
    char c;
    
    // Parse the command line arguments: -h, -v, -s, -E, -b, -t 
    while( (c=getopt(argc,argv,"s:E:b:t:vh")) != -1){
        switch(c){
        case 's':
            s = atoi(optarg);
            break;
        case 'E':
            E = atoi(optarg);
            break;
        case 'b':
            b = atoi(optarg);
            break;
        case 't':
            trace_file = optarg;
            break;
        case 'v':
            verbosity = 1;
            break;
        case 'h':
            printUsage(argv);
            exit(0);
        default:
            printUsage(argv);
            exit(1);
        }
    }

    /* Make sure that all required command line args were specified */
    if (s == 0 || E == 0 || b == 0 || trace_file == NULL) {
        printf("%s: Missing required command line argument\n", argv[0]);
        printUsage(argv);
        exit(1);
    }

    /* Initialize cache */
    initCache();
    replayTrace(trace_file);
    /* Free allocated memory */
    freeCache();
    /* Output the hit and miss statistics for the autograder */
    printSummary(hit_count, miss_count, eviction_count);
    return 0;
}
