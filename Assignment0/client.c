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

#define SEND_BUFFER_SIZE 2048


/* TODO: client()
 * Open socket and send message from stdin.
 * Return 0 on success, non-zero on failure
*/
int client(char *server_ip, char *server_port) {
  int sockfd;
  struct addrinfo hints;
  struct addrinfo *servinfo, *p;
  int rv;
  char send_buffer[SEND_BUFFER_SIZE];
  size_t bytes_read;

  // Step 1: Set up hints
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;        // Use IPv4
  hints.ai_socktype = SOCK_STREAM;  // TCP stream sockets

  // Step 2: Get address info
  if ((rv = getaddrinfo(server_ip, server_port, &hints, &servinfo)) != 0) {
      fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
      return 1;
  }

  // Step 3: Loop through results and connect
  for (p = servinfo; p != NULL; p = p->ai_next) {
      // Create socket
      sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
      if (sockfd == -1) {
          perror("socket");
          continue;
      }

      // Connect to server
      if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
          perror("connect");
          close(sockfd);
          continue;
      }

      break;  // successfully connected
  }

  if (p == NULL) {
      fprintf(stderr, "client: failed to connect\n");
      freeaddrinfo(servinfo);
      return 2;
  }

  freeaddrinfo(servinfo); // Done with address info

  // Step 4: Read from stdin and send in chunks
  while ((bytes_read = fread(send_buffer, 1, SEND_BUFFER_SIZE, stdin)) > 0) {
      size_t total_sent = 0;

      // Handle partial sends
      while (total_sent < bytes_read) {
          ssize_t bytes_sent = send(sockfd, send_buffer + total_sent, bytes_read - total_sent, 0);
          if (bytes_sent == -1) {
              perror("send");
              close(sockfd);
              return 3;
          }
          total_sent += bytes_sent;
      }
  }

  if (ferror(stdin)) {
      perror("fread");
      close(sockfd);
      return 4;
  }

  // Step 5: Close the socket
  close(sockfd);
  return 0;
}

/*
 * main()
 * Parse command-line arguments and call client function
*/
int main(int argc, char **argv) {
  char *server_ip;
  char *server_port;

  if (argc != 3) {
    fprintf(stderr, "Usage: ./client-c [server IP] [server port] < [message]\n");
    exit(EXIT_FAILURE);
  }

  server_ip = argv[1];
  server_port = argv[2];
  return client(server_ip, server_port);
}
