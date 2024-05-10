#include <netdb.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "./shared.h"

void cleanup(void);
void handle_sigint(int sig);

struct addrinfo *server_info;

int main(void) {
  atexit(cleanup);
  signal(SIGINT, handle_sigint);

  // to store the return value of various function calls for error checking
  int rv;

  struct addrinfo hints = {0};     // make sure the struct is empty
  hints.ai_family = AF_UNSPEC;     // don't care whether it's IPv4 or IPv6
  hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
  hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

  // NULL to assign the address of my local host to socket structures
  // because the server is also running locally
  rv = getaddrinfo(NULL, PORT, &hints, &server_info);
  if (rv != 0) {
    fprintf(stderr, "getaddrinfo(): %s\n", gai_strerror(rv));
    return EXIT_FAILURE;
  }

  // create a socket
  int socket_fd = socket(server_info->ai_family, server_info->ai_socktype,
                         server_info->ai_protocol);
  if (socket_fd == -1) {
    perror("socket()");
    return EXIT_FAILURE;
  }

  // connect to the server socket as the address specified in server_info
  rv = connect(socket_fd, server_info->ai_addr, server_info->ai_addrlen);
  if (rv == -1) {
    perror("connect()");
    return EXIT_FAILURE;
  }

  char received_msg[1024];
  char *line = NULL;
  size_t line_cap = 0;
  ssize_t line_len;

  printf("> ");
  while ((line_len = getline(&line, &line_cap, stdin)) > 0) {
    line[line_len - 1] = '\0'; // to ignore the newline character at the end

    // send the message to the server
    int bytes_sent = send(socket_fd, line, line_len, 0);
    if (bytes_sent == -1) {
      perror("send()");
      return EXIT_FAILURE;
    }

    // receive the message from the server
    int bytes_read = recv(socket_fd, received_msg, sizeof received_msg, 0);
    if (bytes_read == -1) {
      perror("recv()");
      return EXIT_FAILURE;
    } else if (bytes_read == 0) {
      fputs("recv(): server closed the connection\n", stderr);
      return EXIT_FAILURE;
    }

    received_msg[bytes_read] = '\0'; // null-terminate the received message

    printf("server says: %s\n", received_msg);
    printf("> ");
  }

  return EXIT_SUCCESS;
}

void cleanup(void) {
  freeaddrinfo(server_info); // free the addrinfo linked list
}

void handle_sigint(int sig) {
  // call exit manually because atexit() registered the cleanup function
  exit(EXIT_SUCCESS);
}