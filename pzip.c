#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <sys/types.h>
#include <stdio.h>
//#include "mythreads=macros.h"
#include <pthread.h>
#include <sys/sysinfo.h>
#define NUMINTS  (10000)
#define FILESIZE (NUMINTS * sizeof(char))
#define WORKINGSIZE (4096)

int FINISH = 0;
int producerqueue;

volatile int balance = 0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t fill = PTHREAD_COND_INITIALIZER;
pthread_cond_t empty = PTHREAD_COND_INITIALIZER;
int buffermax = 64;
int numfull = 0;
int fillptr = 0;
int useptr = 0;
char** output;
int ocount[100000];
typedef struct
{
	char* letters;
	int position;
}work;

work *buffer[64];
void do_fill(work *ptr)
{
	buffer[fillptr] = ptr;
	fillptr = (fillptr + 1) % buffermax;
	numfull++;
}

work* do_get() {
	work* tmp = buffer[useptr];
	useptr = (useptr + 1) % buffermax;
	numfull--;
	return tmp;
}



void* mythread(void* argc)
{
	while(FINISH == 0 || numfull != 0)
	{
		pthread_mutex_lock(&lock);
		while(numfull == 0)
		{
			pthread_cond_wait(&fill, &lock);
		}
		work *temp = do_get();
		char * mymap = temp->letters;
		pthread_cond_signal(&empty);
		pthread_mutex_unlock(&lock);
		//then do work afterwards... print for NOW
		int count = 1;
		char seqChar = mymap[0];
		char *myoutput = malloc((sizeof(int) + sizeof(char)) * WORKINGSIZE);
		int pcount = 0;
		output[temp->position] = myoutput;
		for(int i = 1; i < WORKINGSIZE; i++)
		{
			if(seqChar != mymap[i])
			{
				*((int *)(myoutput + pcount)) = count;
				pcount = pcount + 4;
				*(myoutput + pcount) = seqChar;

				pcount++;
				seqChar = mymap[i];
				count = 1;
			}
			else count++;
		}
		//for the case that all the chars are the same
		if(pcount == 0)
		{
			*((int *)(myoutput + pcount)) = count;
			pcount = pcount + 4;
			*(myoutput + pcount) = seqChar;
			pcount++; 
		}
		ocount[temp->position] = pcount;
		free(temp);
	}
	return 0;

}
//check if the last character of one output is the same as the first of the next output
int fixboundaries()
{
	int k = 0;
	while(*(output + k + 1) != NULL)
	{
		char* temp = *(output + k);
		char* temp2 = *(output + k + 1);
		//  printf("%c = %c\n", *(temp + ocount[k] - 1), *(temp2 + 4));
		if(*(temp + ocount[k] - 1) == *(temp2 + 4))
		{
			//  printf("got one \n");
			int value = (int) *temp2;
			int add = (int) *(temp + ocount[k] - 5);
			*(temp + ocount[k] - 5) = 0;
			ocount[k] = ocount[k] - 5;
			value = value + add;
			*((int *)(temp2)) = value;
		} 
		k++;
	}
	return k;
}

int main(int argc, char *argv[])
{
	int fd;
	int pagesize = getpagesize();
	char* fullmap[1000000];
	//    char * map;  /* mmapped array of chars */
	int mapcounter = 0;
	for(int files = 1; files < argc; files++)
	{
		fd = open(argv[files], O_RDONLY);
		if (fd == -1) {
			perror("Error opening file for reading");
			exit(EXIT_FAILURE);
		}
		struct stat *tempstat = malloc(sizeof(struct stat));
		if(fstat(fd, tempstat) != 0)
		{
			close(fd);
			perror("Error getting stat");
			exit(EXIT_FAILURE);
		}
		size_t filesize = tempstat->st_size;
                printf("filesize %lu \n", filesize);
		int memcount = 0;
		while(memcount < filesize)
		{
			fullmap[mapcounter] = mmap(0, pagesize, PROT_READ, MAP_SHARED, fd, 0);
			if (fullmap[mapcounter] == MAP_FAILED) {
				close(fd);
				perror("Error mmapping the file");
				exit(EXIT_FAILURE);
			}
			memcount = memcount + pagesize;
			mapcounter++;
			
		}
		close(fd);
	}
	output = malloc(sizeof(char*) * mapcounter);
	int procs = get_nprocs();
	pthread_t threads[procs];
	for(int i = 0; i < procs; i++)
	{
		pthread_create(&threads[i], NULL, mythread, NULL);
	}
	producerqueue = 0;
	//running producer queue
	for(int nfiles = 0; nfiles < mapcounter; nfiles++)
	{
		for(int count = 0; count < (pagesize / WORKINGSIZE); count++)
		{
			pthread_mutex_lock(&lock);
			while(numfull == buffermax)
			{
				pthread_cond_wait(&empty, &lock);
			}
			work *temp = malloc(sizeof(work));
			temp->letters = fullmap[nfiles];
			temp->position = producerqueue;
			producerqueue++;
			do_fill(temp);
			pthread_cond_signal(&fill);
			pthread_mutex_unlock(&lock);
		}
	}
	//what if it finishes sending out work before threads are done working on them?
	FINISH = 1;
	for(int i = 0; i < procs; i++){
		pthread_join(threads[i], NULL);
	}

	//NOW three threads, OSscheduler:which to run?
	//join waits for the threads to finish
	fixboundaries();
	//printf("break\n \n:");
	int k = 0;
	while(*(output + k) != NULL)
	{
		char* temp = *(output + k);
		fwrite(temp, sizeof(int) + sizeof(char), ocount[k], stdout);
		k++;
	}

	/*
	   if (munmap(map, FILESIZE) == -1) {
	   perror("Error un-mmapping the file");
	   }
	 */

	return 0;
}
