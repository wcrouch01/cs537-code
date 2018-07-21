/*
 * basic tests for thread_create() and thread_join()
 * Authors:
 * - Varun Naik, Spring 2018
 */
#include "types.h"
#include "stat.h"
#include "user.h"

#define PGSIZE 0x1000
#define check(exp, msg) if(exp) {} else {\
  printf(1, "%s:%d check (" #exp ") failed: %s\n", __FILE__, __LINE__, msg);\
  printf(1, "TEST FAILED\n");\
  kill(ppid);\
  exit();}

int ppid = 0;
volatile int cpid = 0;
volatile int global = 0;

static inline uint rdtsc() {
  uint lo, hi;
  asm("rdtsc" : "=a" (lo), "=d" (hi));
  return lo;
}

void
func(void *arg1, void *arg2)
{
  volatile int retval;

  // Assign retval to return value of clone()
  asm volatile("movl %%eax,%0" : "=r"(retval));

  check(*(int *)arg1 == 0xABCDABCD, "*arg1 is incorrect");
  check(*(int *)arg2 == 0xCDEFCDEF, "*arg2 is incorrect");
  check(((uint)&retval % PGSIZE) == 0xFE4, "Local variable is in wrong location");
  check(retval == 0, "Return value of clone() in child is incorrect");

  // Change external state
  *(int *)arg1 = 0x12341234;
  cpid = getpid();
  check(cpid > ppid, "getpid() returned the wrong pid");
  ++global;

  exit();

  check(0, "Continued after exit");
}

int
main(int argc, char *argv[])
{
  int arg1 = 0xABCDABCD;
  int arg2 = 0xCDEFCDEF;
  int pid1, pid2, status;
  void *unused;

  ppid = getpid();
  check(ppid > 2, "getpid() failed");

  // With the given allocator, after this line, malloc() will (probably) not be
  // page aligned
  unused = malloc(rdtsc() % (PGSIZE-1) + 1);
  printf(2, "print1");
  pid1 = thread_create(&func, &arg1, &arg2);
  check(pid1 > ppid, "thread_create() failed");
  printf(2, "print2");
  pid2 = thread_join();
  printf(2, "print3");
  status = kill(pid1);
  printf(2, "print4");
  check(status == -1, "Child was still alive after thread_join()");
  check(pid1 == pid2, "thread_join() returned the wrong pid");
  check(arg1 == 0x12341234, "arg1 is incorrect");
  check(arg2 == 0xCDEFCDEF, "arg2 is incorrect");
  check(cpid == pid1, "cpid is incorrect");
  check(global == 1, "global is incorrect");
  printf(2, "print5");
  free(unused);
  printf(1, "PASSED TEST!\n");
  exit();
}
