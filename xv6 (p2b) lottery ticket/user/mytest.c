#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"
#include "syscall.h"
#include "traps.h"


int main() {
  struct pstat ps;
   
  getpinfo(&ps);
  for(int i = 0; i < 64; i++) {
 	 printf(ps.tickets[i]);
         settickets(10);
         printf(ps.tickets[i]);
}
   

}
