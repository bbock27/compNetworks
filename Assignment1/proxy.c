#include "proxy_parse.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <errno.h>
#include <netinet/tcp.h>
#include <signal.h>

void sigchld_handler(int s)
{
    (void)s; // quiet unused variable warning

    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}

int setUpConnection(char *proxy_port){
  struct addrinfo hints, *res;
  int sfd;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  if (getaddrinfo(NULL, proxy_port, &hints, &res) != 0) {
      perror("getaddrinfo");
      return -1;
  }

  sfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  if (sfd < 0) { perror("socket"); freeaddrinfo(res); return -1; }

  int yes = 1;
  setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

  if (bind(sfd, res->ai_addr, res->ai_addrlen) < 0) {
      perror("bind");
      close(sfd);
      freeaddrinfo(res);
      return -1;
  }
  int queueSize = 10;
  if (listen(sfd, queueSize) < 0) {
      perror("listen");
      close(sfd);
      freeaddrinfo(res);
      return -1;
  }

  freeaddrinfo(res);
  return sfd;

}


// Read until \r\n\r\n or buffer full. Returns bytes read or -1
int readReqLine(int fd, char *buf, size_t bufsz) {
  int off = 0;
  while (off + 1 < bufsz) {
    int r = read(fd, buf + off, 1);
    if (r == 0) return off; // eof
    if (r < 0) {
      if (errno == EINTR) continue;
      return -1;
    }
    off += r;
    if (off >= 4 && buf[off-4] == '\r' && buf[off-3] == '\n' && buf[off-2] == '\r' && buf[off-1] == '\n') {
      return off;
    }
  }
  return off; //buffer full
}


/*
Checks the validity of a request line in buf of size bufSize
returns:
0 = valid
1 = not GET
2 = invalid format
*/
int checkReqLineValidity(char*buf, size_t bufSize){
  
  int spaceCount = 0;
  for(int i = 0; i < bufSize; i++){
    if(buf[i] == ' '){
      spaceCount += 1;
    }
  }



  if(spaceCount != 2){
    return 2;
  }

  if(strncmp(buf, "GET", 3) != 0){
    return 1;    
  }

  if (bufSize >= 4 && buf[bufSize-4] == '\r' && buf[bufSize-3] == '\n' && buf[bufSize-2] == '\r' && buf[bufSize-1] == '\n') {
    return 0;
  }
  else{
    return 2;
  }

}

int readHeaders(int fd, char *buf, size_t bufsz, int startPoint){
  int bytesRead = 0;
  while (startPoint + bytesRead + 1 < bufsz) {
    int r = read(fd, buf + startPoint + bytesRead, 1);
    if (r == 0) {
      return bytesRead; // eof
    }
    if (r < 0) {
      if (errno == EINTR) continue;
      return -1;
    }
    bytesRead += r;
    
  }
  return bytesRead; //buffer full
}


int handle_client(int clientfd){

  /* 
  NOW IN NEW PROCESS
  
  parse http request

  construct and send request to remote server

  relay the response to the client
  */


  //read in from clientfd
  struct ParsedRequest* request = ParsedRequest_create();
  int bufSize = 1024*16;
  char buf[bufSize+1];
  readReqLine(clientfd, buf, bufSize);

  int bufLenUsed = strlen(buf);

  int reqLineValidity = checkReqLineValidity(buf, bufLenUsed);

  if(reqLineValidity == 1){
    //send not implemented response
  }
  else if(reqLineValidity == 2){
    //send bad request error
  }

  //at this point we have a properly formnatted req line
  int response = readHeaders(clientfd, buf, bufSize, bufLenUsed);


  if (ParsedRequest_parse(request, buf, bufLenUsed) < 0) {
    printf("parse failed\n");
    return -1;
  }

  printf("yay!%s", buf);
}

/* TODO: proxy()
 * Establish a socket connection to listen for incoming connections.
 * Accept each client request in a new process.
 * Parse header of request and get requested URL.
 * Get data from requested remote server.
 * Send data to the client
 * Return 0 on success, non-zero on failure
*/
int proxy(char *proxy_port) {

  //set up sig handler to listen for SIGCHLD signal and check if child responsible terminated
  struct sigaction sa;
  sa.sa_handler = sigchld_handler; // reap all dead processes
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if (sigaction(SIGCHLD, &sa, NULL) == -1) {
    perror("sigaction");
    return 1;
  }

  //establish socket connection and listen for incomming connections
  //get my ip addr
  //listen on my ip at port proxy_port

  

  int listenFd = setUpConnection(proxy_port);
  if (listenFd < 0) {
    fprintf(stderr, "Failed to create listening socket on port %s\n", proxy_port);
    return 1;
  }
  fprintf(stderr, "Proxy listening on port %s\n", proxy_port);
  

  //on new connection, fork
  while(1) {

    struct sockaddr_storage cliaddr;
    socklen_t cli_len = sizeof(cliaddr);
    int clientfd = accept(listenFd, (struct sockaddr*)&cliaddr, &cli_len);
    if (clientfd < 0) {
      if (errno == EINTR) continue;
      perror("accept");
      break;
    }

    pid_t pid = fork();
    if (pid < 0) {
      perror("fork");
      close(clientfd);
      continue;
    } else if (pid == 0) {
      //child process
      close(listenFd); //dont need bc we're in child so not listening for any new connections
      handle_client(clientfd);
      return 0;
    } else {
      //parent process
      close(clientfd);
    }
  }

  
}


int main(int argc, char * argv[]) {
  char *proxy_port;

  if (argc != 2) {
    fprintf(stderr, "Usage: ./proxy <port>\n");
    exit(EXIT_FAILURE);
  }

  proxy_port = argv[1];
  return proxy(proxy_port);
}
