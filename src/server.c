#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "./shared.h"

// the max number of connections that can wait in the queue
#define BACKLOG 10

void cleanup(void);
void handle_sigint(int sig);
void handle_client_message(int conn_fd);

// to store the address information of the server
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
  rv = getaddrinfo(NULL, PORT, &hints, &server_info);
  if (rv != 0) {
    fprintf(stderr, "getaddrinfo(): %s\n", gai_strerror(rv));
    return EXIT_FAILURE;
  }

  // create a socket, which apparently is no good by itself because it's not
  // bound to an address and port number
  int socket_fd = socket(server_info->ai_family, server_info->ai_socktype,
                         server_info->ai_protocol);
  if (socket_fd == -1) {
    perror("socket()");
    return EXIT_FAILURE;
  }

  // lose the "Address already in use" error message. why this happens
  // in the first place? well even after the server is closed, the port
  // will still be hanging around for a while, and if you try to restart the
  // server, you'll get an "Address already in use" error message
  int yes = 1;
  if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1) {
    perror("setsockopt()");
    exit(1);
  }

  // bind the socket to the address and port number specified in server_info
  rv = bind(socket_fd, server_info->ai_addr, server_info->ai_addrlen);
  if (rv == -1) {
    perror("bind()");
    return EXIT_FAILURE;
  }

  // time to listen for incoming connections
  // BACKLOG is the max number of connections that can wait in the queue
  rv = listen(socket_fd, BACKLOG);
  if (rv == -1) {
    perror("listen()");
    return EXIT_FAILURE;
  }

  printf("server is listening on port %s...\n", PORT);

  struct sockaddr_storage their_addr;
  socklen_t addr_size = sizeof their_addr;
  while (true) {
    int conn_fd = accept(socket_fd, (struct sockaddr *)&their_addr, &addr_size);
    if (conn_fd == -1) {
      perror("accept()");
      return EXIT_FAILURE;
    }

    printf("client %d connected\n", conn_fd);

    // since this is a blocking function, we'll only be able to handle one
    // client at a time
    handle_client_message(conn_fd);
  }

  return EXIT_SUCCESS;
}

/**
 * this is a blocking fuction that handles the messages from the client
 */
void handle_client_message(int conn_fd) {
  while (true) {
    char received_msg[1024];
    int bytes_read = recv(conn_fd, received_msg, sizeof received_msg - 1, 0);
    if (bytes_read == -1) {
      perror("handle_client_message(): recv()");
      return;
    } else if (bytes_read == 0) {
      puts("handle_client_message(): Client disconnected");
      return;
    }

    received_msg[bytes_read] = '\0';

    printf("client %d says: %s\n", conn_fd, received_msg);

    char reply[1024] = "you said: \0";

    // concat the received message to the reply message
    strncat(reply, received_msg, sizeof reply - strlen(reply) - 1);

    int bytes_sent = send(conn_fd, reply, strlen(reply), 0);
    if (bytes_sent == -1) {
      perror("handle_client_message(): send()");
      return;
    }
  }

  close(conn_fd);
}

void cleanup(void) {
  freeaddrinfo(server_info); // free the addrinfo linked list
}

void handle_sigint(int sig) {
  // call exit manually because atexit() registered the cleanup function
  exit(EXIT_SUCCESS);
}