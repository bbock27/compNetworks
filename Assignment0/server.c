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

#define QUEUE_LENGTH 10
#define RECV_BUFFER_SIZE 2048

/* TODO: server()
 * Open socket and wait for client to connect
 * Print received message to stdout
 * Return 0 on success, non-zero on failure
*/
int server(char *server_port) {
  int listen_fd, client_fd;
  struct addrinfo hints, *servinfo, *p;
  int rv;
  char recv_buffer[RECV_BUFFER_SIZE];

  // Step 1: Set up hints for IPv4 and TCP
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;        // IPv4
  hints.ai_socktype = SOCK_STREAM;  // TCP
  hints.ai_flags = AI_PASSIVE;      // Use my IP (INADDR_ANY)

  // Step 2: Get address info for binding
  if ((rv = getaddrinfo(NULL, server_port, &hints, &servinfo)) != 0) {
      fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
      return 1;
  }

  // Step 3: Loop through results and bind to the first available
  for (p = servinfo; p != NULL; p = p->ai_next) {
      listen_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
      if (listen_fd == -1) {
          perror("socket");
          continue;
      }

      int yes = 1;
      if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
          perror("setsockopt");
          close(listen_fd);
          freeaddrinfo(servinfo);
          return 2;
      }

      if (bind(listen_fd, p->ai_addr, p->ai_addrlen) == -1) {
          perror("bind");
          close(listen_fd);
          continue;
      }

      break; // successfully bound
  }

  if (p == NULL) {
      fprintf(stderr, "server: failed to bind\n");
      freeaddrinfo(servinfo);
      return 3;
  }

  freeaddrinfo(servinfo); // Done with address info

  // Step 4: Listen
  if (listen(listen_fd, QUEUE_LENGTH) == -1) {
      perror("listen");
      close(listen_fd);
      return 4;
  }

  // Step 5: Accept connections in a loop
  while (1) {
      client_fd = accept(listen_fd, NULL, NULL);
      if (client_fd == -1) {
          perror("accept");
          continue; // don't crash on accept error
      }

      // Step 6: Read data from client and write to stdout
      ssize_t bytes_received;
      while ((bytes_received = recv(client_fd, recv_buffer, RECV_BUFFER_SIZE, 0)) > 0) {
          fwrite(recv_buffer, 1, bytes_received, stdout);
          fflush(stdout);  // flush output for each chunk
      }

      if (bytes_received == -1) {
          perror("recv"); // don't exit; just print error
      }

      close(client_fd); // done with this client
  }

  // Never reached, but good practice
  close(listen_fd);
  return 0;
}

/*
 * main():
 * Parse command-line arguments and call server function
*/
int main(int argc, char **argv) {
  char *server_port;
  printf("start");

  if (argc != 2) {
    fprintf(stderr, "Usage: ./server-c [server port]\n");
    exit(EXIT_FAILURE);
  }

  server_port = argv[1];
  return server(server_port);
}
