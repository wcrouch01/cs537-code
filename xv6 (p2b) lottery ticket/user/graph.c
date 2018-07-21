#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"
#include "syscall.h"
#include "traps.h"
#include "pstat.h"
#include "param.h"
void
spin()
{       
        int i = 0;
  int j = 0;
  int k = 0;
        for(i = 0; i < 50; ++i)
        {       
                for(j = 0; j < 400000; ++j)
                {       
                        k = j % 10;
      k = k + 1;
     }  
        }
}

void print(struct pstat *st, int data)
{  
   int i;
   printf(1, "%d ", data);
   for(i = 0; i < NPROC; i++) {
      if (st->inuse[i] && st->tickets[i] > 9) {
          printf(1, "%d ", st->ticks[i]);
      }
   }
   printf(1, "\n");
}
int
main(int argc, char *argv[])
{
  int tickets = 10;
  struct pstat pgraph;
  int procid;
  for(int i = 0; i < 3; i++){
    procid = fork();
    if(procid == 0)
      {
        settickets(tickets);
        break;
      }
    tickets = tickets + 10;
   }
   sleep(1);
   if(procid != 0)
   {
     for(int data = 0; data < 150; data++)
       {
          if(getpinfo(&pgraph) == 0)
	   	print(&pgraph, data);
        sleep(1);
   }
   return 0;
}
   //spin
   else
   {
      for(;;)
      {
        spin();
      }


   }



}

