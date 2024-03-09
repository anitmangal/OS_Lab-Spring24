#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main ()
{
   int n, mypid, parpid;

   printf("Parent: n = "); scanf("%d", &n);

   /* Child creation */
   if (fork() == 0) {      /* Child process */
      mypid = getpid();
      parpid = getppid();
      printf("Child : PID = %u, PPID = %u...\n", mypid, parpid);
      n = n * n;
   }
   else { /* Parent process */
      mypid = getpid();
      parpid = getppid();
      printf("Parent: PID = %u, PPID = %u...\n", mypid, parpid);
   } 

   printf("Process PID = %u: n = %d\n", mypid, n);

   exit(0);
}
