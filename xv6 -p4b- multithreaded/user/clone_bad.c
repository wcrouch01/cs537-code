/*
 * error tests for clone() and join()
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

void
func(void *arg1, void *arg2)
{
  if (global != 1) {
    check(0, "Should not be called");
  }

  // Change external state
  cpid = getpid();
  check(cpid > ppid, "getpid() returned the wrong pid");
  ++global;

  exit();

  check(0, "Continued after exit");
}

int
main(int argc, char *argv[])
{
  void *stack1, *stack2;
  char *limit;
  int pid1, pid2, status;

  ppid = getpid();
  check(ppid > 2, "getpid() failed");

  // Expand address space for stack
  stack1 = sbrk(PGSIZE);
  check(stack1 != (char *)-1, "sbrk() failed");
  check((uint)stack1 % PGSIZE == 0, "sbrk() return value is not page aligned");
  limit = (char *)stack1 + PGSIZE;

  // Pass pointer outside the address space to clone()
  pid1 = clone((void *)limit, NULL, NULL, stack1);
  check(pid1 == -1, "clone() returned the wrong pid");
  pid1 = clone(&func, NULL, NULL, limit);
  check(pid1 == -1, "clone() returned the wrong pid");

  // Pass a misaligned stack to clone()
  pid1 = clone(&func, NULL, NULL, (char *)stack1-1);
  check(pid1 == -1, "clone() returned the wrong pid");

  // Expand address space, but not enough for another stack
  limit = sbrk(PGSIZE-1);
  check(limit == (char *)stack1 + PGSIZE, "sbrk() returned the wrong value");
  pid1 = clone(&func, NULL, NULL, limit);
  check(pid1 == -1, "clone() returned the wrong pid");
  printf(2, "gothere1");
  limit = sbrk(-PGSIZE+1);
  check(limit == (char *)stack1 + 2*PGSIZE-1, "sbrk() returned the wrong value");
  limit = (char *)stack1 + PGSIZE;
  printf(2, "gothere2");
  // Try to join() before any successful clone()
  pid2 = join(&stack2);
  printf(2, "gothere2.5");
  check(pid2 == -1, "join() returned the wrong pid");
  printf(2, "gothere3");
  ++global;
  pid1 = clone(&func, NULL, NULL, stack1);
  check(pid1 > ppid, "clone() failed");

  // Pass pointer outside the address space to join()
  pid2 = join((void **)(limit - sizeof(void *) + 1));
  check(pid2 == -1, "join() returned the wrong pid");
  printf(2, "gothere4");
  // Argument to join() is pointer to last 4-byte address in address space
  pid2 = join((void **)(limit - sizeof(void *)));
  status = kill(pid1);
  printf(2, "gothere4");
  check(status == -1, "Child was still alive after join()");
  check(pid1 == pid2, "join() returned the wrong pid");
  check(stack1 == *(void **)(limit - sizeof(void *)), "join() returned the wrong stack");
  check(cpid == pid1, "cpid is incorrect");
  check(global == 2, "global is incorrect");

  printf(1, "PASSED TEST!\n");
  exit();
}
