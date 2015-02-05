#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>

using namespace std;

#define BUF_SIZE 1000000

static __inline__ unsigned long long rdtsc() {
    unsigned int low, high;
    __asm__ __volatile__ (
        "xorl %%eax,%%eax \n    cpuid"
        ::: "%rax", "%rbx", "%rcx", "%rdx" );
    __asm__ __volatile__ (
                          "rdtsc" : "=a" (low), "=d" (high));
    return (unsigned long long)high << 32 | low;
}
 
void usePipe(char* msg)
{
     int pC2P[2];      /* Pipe file descriptors child write to parent */
     int pP2C[2];      /* parent write to child */
     char buf[BUF_SIZE];
     ssize_t numRead;
 
     if (pipe(pC2P)==-1 || pipe(pP2C)==-1 )  /* Create the pipe */
         perror("pipe"); 

     switch (fork()) {
     case -1:
         perror("fork");
 
     case 0:             /* Child  - reads from pipe */
         if (close(pP2C[1]) == -1)            /* Write end is unused */
             perror("close - parent write");
 
         for (;;) {              /* Read data from pipe, echo on stdout */
             numRead = read(pP2C[0], buf, BUF_SIZE);
             if (numRead == -1)
                 perror("read");
             if (numRead == 0)
                 break;                      /* End-of-file */
         }
 
         //write(STDOUT_FILENO, "\n", 1);
	 if( close(pP2C[0])==-1 ) 
	     perror("close - child read");
         if( close(pC2P[0])==-1 )
	     perror("close - parent read");
         if (write(pC2P[1], msg, strlen(msg)) != strlen(msg))
             perror("child - partial/failed write");
        
         if( close(pC2P[1])==-1 )
	     perror("close - child write");
	 return;
     default:            /* Parent - writes to pipe */
         if (close(pP2C[0]) == -1)            /* Read end is unused */
             perror("close - child read");
 
         if (write(pP2C[1], msg, strlen(msg)) != strlen(msg))
             perror("parent - partial/failed write");
 
         if (close(pP2C[1]) == -1)            /* Child will see EOF */
             perror("close - parent write");

         wait(NULL);                         /* Wait for child to finish */
     	 
	 if(close(pC2P[1] == -1) ) // read the same content
	     perror("close child write");
         for (;;) {              /* Read data from pipe, echo on stdout */
             numRead = read(pC2P[0], buf, BUF_SIZE);
             if (numRead == -1)
                 perror("read");
             if (numRead == 0)
		 break;
         }
	 if(close(pC2P[0]) == -1)
	     perror("close parent read");         

	 return;
     }
 }

int main() {
    unsigned long long t0 = rdtsc();
    sleep(1);
    unsigned long long t1 = rdtsc();

    char* msg = "hello pipe";
    usePipe(msg);

    timespec* pTT0 = new timespec();
    timespec* pTT1 = new timespec();
    clock_gettime(CLOCK_REALTIME, pTT0);
    sleep(2);
    clock_gettime(CLOCK_REALTIME, pTT1);
    printf("%ld %ld\n", pTT1->tv_sec-pTT0->tv_sec, pTT1->tv_nsec-pTT0->tv_nsec);


    timeval* pT0 = new timeval();
    timeval* pT1 = new timeval();
    gettimeofday(pT0, NULL);
    sleep(2);
    gettimeofday(pT1, NULL);
    printf("%llu - %llu = %llu\n", t1, t0, t1-t0 );
    printf("%ld %ld\n", pT0->tv_sec, pT0->tv_usec);
    printf("%ld %ld\n", pT1->tv_sec, pT1->tv_usec);
    printf("%ld %ld\n", pT1->tv_sec-pT0->tv_sec, 
                        pT1->tv_usec-pT0->tv_usec);
    return 0;
}
