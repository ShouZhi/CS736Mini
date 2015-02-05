#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <arpa/inet.h>

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

int serverSockfd, serverNewSockfd, clientSockfd;

int createServerSocket(int portNum) {
    int sockfd, portno;
    struct sockaddr_in serv_addr;
    int n;
  
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        perror("perror opening socket");
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = portNum;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *) &serv_addr,
             sizeof(serv_addr)) < 0) 
             perror("perror on binding");
   
   return sockfd;
}

int listenClient(int sockfd) {
    int newsockfd;                       
    listen(sockfd,5);
    struct sockaddr_in cli_addr;
    socklen_t clilen = sizeof(cli_addr);
    newsockfd = accept(sockfd, 
                (struct sockaddr *) &cli_addr, 
                &clilen); 
    return newsockfd;
}

void send( int newsockfd, char* msg, int msgLen ) {
    if( write(newsockfd, msg, msgLen) < 0 )
         perror("error writing to socket");
}

void recv( int newsockfd, char* buffer, int limit ) {
    if( read(newsockfd, buffer, limit) )
        perror("perror reading from socket");
}

void closeServerSocket(int newsockfd, int sockfd) {
     close(newsockfd);
     close(sockfd);
}

int createClientSocket(int portNum, char* server_str)
{
    int sockfd, portno, n; 
    struct sockaddr_in serv_addr;
    
    portno = portNum;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        perror("perror opening socket");
    
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_addr.s_addr = inet_addr(server_str);
    serv_addr.sin_port = htons(portno);

    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
        perror("perror connecting");
    return sockfd;
}

void closeClientSocket( int sockfd ) {
    close(sockfd);
}


int main() {
    unsigned long long t0;
    unsigned long long t1;
    timespec* pT0 = new timespec();
    timespec* pT1 = new timespec();
    timeval* pTT0 = new timeval();
    timeval* pTT1 = new timeval();
    char* msg = new char[1<<21];
    char* buffer = new char[1<<21];
    
    int msgLen = 4;
    int procId;
    switch( (procId = fork()) ) {
        case -1:
            perror("ford fail");
            break;
        case 0: // child client
            clientSockfd = createClientSocket( 2000, "127.0.0.1" );
            send( clientSockfd, msg, msgLen );
            recv( clientSockfd, buffer, msgLen );
            break;    
        default: // parent server 
            serverSockfd = createServerSocket( 2000 );
            serverNewSockfd = listenClient( serverSockfd );
            recv( serverNewSockfd, buffer, msgLen );
            send( serverNewSockfd, msg, msgLen );
    }
   
    sleep(2);  

    switch( procId ) {
        case 0:        
            recv( clientSockfd, buffer, msgLen );
            send( clientSockfd, msg, msgLen );
            break;
        default:
            clock_gettime( CLOCK_REALTIME, pT0 );
            send( serverNewSockfd, msg, msgLen );
            recv( serverNewSockfd, buffer, msgLen );
            clock_gettime( CLOCK_REALTIME, pT1 );
    }

    
    switch( procId ) {
        case 0:
            closeClientSocket( clientSockfd );
            break;
        default:
            closeServerSocket( serverSockfd, serverNewSockfd ); 
            printf("%lld\n", ((long long)(pT1->tv_sec-pT0->tv_sec))*1000000000 + pT1->tv_nsec-pT0->tv_nsec);
    }
    return 0; 
}
