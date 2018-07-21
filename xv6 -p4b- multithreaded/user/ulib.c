#include "types.h"
#include "stat.h"
#include "fcntl.h"
#include "user.h"
#include "x86.h"



 void lock_init(lock_t *lock) {
 lock->ticket = 0;
 lock->turn = 0;
 }

   void lock_acquire(lock_t *lock) {
   int myturn = fetch_and_add(&lock->ticket, 1);
     while (lock->turn != myturn)
       ; // spin
 }

 void lock_release(lock_t *lock) {
 lock->turn = lock->turn + 1;
 }
lock_t *ourlock;

void** usermem[64][2];
/*
static inline int fetch_and_add(int* variable, int value)
  {
      __asm__ volatile("lock; xaddl %0, %1"
        : "+r" (value), "+m" (*variable) // input+output
        : // No input-only
        : "memory"
      );
      return value;
  }
*/

char*
strcpy(char *s, char *t)
{
  char *os;

  os = s;
  while((*s++ = *t++) != 0)
    ;
  return os;
}

int
strcmp(const char *p, const char *q)
{
  while(*p && *p == *q)
    p++, q++;
  return (uchar)*p - (uchar)*q;
}

uint
strlen(char *s)
{
  int n;

  for(n = 0; s[n]; n++)
    ;
  return n;
}

void*
memset(void *dst, int c, uint n)
{
  stosb(dst, c, n);
  return dst;
}

char*
strchr(const char *s, char c)
{
  for(; *s; s++)
    if(*s == c)
      return (char*)s;
  return 0;
}

char*
gets(char *buf, int max)
{
  int i, cc;
  char c;

  for(i=0; i+1 < max; ){
    cc = read(0, &c, 1);
    if(cc < 1)
      break;
    buf[i++] = c;
    if(c == '\n' || c == '\r')
      break;
  }
  buf[i] = '\0';
  return buf;
}

int
stat(char *n, struct stat *st)
{
  int fd;
  int r;

  fd = open(n, O_RDONLY);
  if(fd < 0)
    return -1;
  r = fstat(fd, st);
  close(fd);
  return r;
}

int
atoi(const char *s)
{
  int n;

  n = 0;
  while('0' <= *s && *s <= '9')
    n = n*10 + *s++ - '0';
  return n;
}

void*
memmove(void *vdst, void *vsrc, int n)
{
  char *dst, *src;
  
  dst = vdst;
  src = vsrc;
  while(n-- > 0)
    *dst++ = *src++;
  return vdst;
}

int thread_create(void (*start_routine)(void *, void *), void *arg1, void *arg2)
{
 if(ourlock == NULL)
 {
   lock_init(ourlock);
 }
 lock_acquire(ourlock);
 void* stack;
 stack = malloc(8192);
 if(stack == NULL)
 {
   lock_release(ourlock);
   return -1;
 }
 int i = 0;
 while(usermem[i][0] != NULL)
    i++;
 usermem[i][0] = stack;
 usermem[i][1] = (void *) PGROUNDUP((int) stack);
 int pid = clone(start_routine, arg1, arg2, usermem[i][1]);
 lock_release(ourlock);
 return pid;

}

int thread_join()
{
 lock_acquire(ourlock);
 void* stack;
 int mystack = join(&stack);
 if(mystack == -1)
 {
   lock_release(ourlock);
   return -1;
 }
 //printf(1, "== after join in thread_join\n");
 for(int i = 0; i < 64; i++)
 {
   if(stack == usermem[i][1]){
      //printf(1, "=== check here %d\n", i);
      free(usermem[i][0]);
      usermem[i][0] = NULL;
      break;
   }
 }
 lock_release(ourlock);
 //printf(1, "=== to return\n");
 return mystack;

}

