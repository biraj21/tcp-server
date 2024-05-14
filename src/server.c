#include <netdb.h>      // for getaddrinfo()
#include <poll.h>       // for poll()
#include <signal.h>     // for signal()
#include <stdbool.h>    // for true/false, duh
#include <stddef.h>     // for size_t
#include <stdio.h>      // for printf(), perror(), fprintf(), puts()
#include <stdlib.h>     // for EXIT_SUCCESS, EXIT_FAILURE, atexit(), exit()
#include <string.h>     // for strncat(), strlen()
#include <sys/socket.h> // for socket(), bind(), listen(), accept(), recv(), send()
#include <unistd.h>     // for close()

#include "./shared.h"
#include "./vector.h"

// the max number of connections that can wait in the queue
#define BACKLOG 5

enum ConnectionState {
  CONN_STATE_REQ = 0,
  CONN_STATE_RES,
  CONN_STATE_END,
};

struct Connection {
  int fd;
  enum ConnectionState state;
  char read_buffer[1024];
  char write_buffer[1024];
};

static bool accept_new_connection(void);
static void handle_connection_io(struct Connection *conn);

static void cleanup(void);
static void handle_sigint(int sig);

// to store the address information of the server
struct addrinfo *server_info = NULL;

// the socket file descriptor
int socket_fd = -1;

// a vector of Connection structs to store the active connections
struct Vector *connections;

// a vector of pollfd structs to store the file descriptors that we want to
// poll for events
struct Vector *poll_fds = NULL;

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

  // loop through all the results and bind to the first we can
  struct addrinfo *p;
  for (p = server_info; p != NULL; p = p->ai_next) {
    // create a socket, which apparently is no good by itself because it's not
    // bound to an address and port number
    socket_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (socket_fd == -1) {
      perror("socket()");
      continue;
    }

    // lose the "Address already in use" error message. why this happens
    // in the first place? well even after the server is closed, the port
    // will still be hanging around for a while, and if you try to restart
    // the server, you'll get an "Address already in use" error message
    int yes = 1;
    rv = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);
    if (rv == -1) {
      perror("setsockopt()");
      exit(EXIT_FAILURE);
    }

    // bind the socket to the address and port number
    rv = bind(socket_fd, p->ai_addr, p->ai_addrlen);
    if (rv == -1) {
      close(socket_fd);
      perror("bind()");
      continue;
    }

    // binding was successful, so break out of the loop
    break;
  }

  // check if we were able to bind to an address and port number
  if (p == NULL) {
    fprintf(stderr, "failed to bind\n");
    exit(EXIT_FAILURE);
  }

  // free server_info because we don't need it anymore
  freeaddrinfo(server_info);
  server_info = NULL; // to avoid dangling pointer (& double free at cleanup())

  // time to listen for incoming connections
  // BACKLOG is the max number of connections that can wait in the queue
  rv = listen(socket_fd, BACKLOG);
  if (rv == -1) {
    perror("listen()");
    return EXIT_FAILURE;
  }

  printf("server is listening on port %s...\n", PORT);

  // initialize connections vector
  connections = vector_init(sizeof(struct Connection), 0);

  // initialize the poll_fds vector
  poll_fds = vector_init(sizeof(struct pollfd), 0);

  // the event loop
  while (true) {
    // clear the poll_fds vector
    vector_clear(poll_fds);

    // initialize the pollfd struct for the socket file descriptor
    struct pollfd socket_pfd = {socket_fd, POLLIN, 0};
    vector_push(poll_fds, &socket_pfd);

    size_t num_connections = vector_length(connections);
    for (size_t i = 0; i < num_connections;) {
      struct Connection *conn = vector_get(connections, i);
      if (conn->state == CONN_STATE_END) {
        // if the connection is in the end state, close the connection
        close(conn->fd);

        // replace the current connection with the last connection in the vector
        if (i != num_connections - 1) {
          vector_set(connections, i,
                     vector_get(connections, num_connections - 1));
        }

        // remove the last connection from the vector
        vector_pop(connections);

        // decrement the number of connections
        --num_connections;

        continue;
      }

      // create pollfd struct and push it to the poll_fds vector
      struct pollfd pfd = {
          .fd = conn->fd,
          .events = (conn->state == CONN_STATE_REQ) ? POLLIN : POLLOUT,
          .revents = 0,
      };
      pfd.events = pfd.events | POLLERR;
      vector_push(poll_fds, &pfd);

      ++i;
    }

    // poll for active fds
    rv = poll(vector_data(poll_fds), vector_length(poll_fds), 0);
    if (rv == -1) {
      perror("poll()");
    }

    // process active connections
    size_t num_poll_fds = vector_length(poll_fds);
    for (size_t i = 1; i < num_poll_fds; ++i) {
      // skipped the first pollfd because it's the socket file descriptor

      struct pollfd *pfd = vector_get(poll_fds, i);
      if (pfd->revents) {
        struct Connection *conn = vector_get(connections, i - 1);

        if (conn == NULL) {
          // this should never happen, but just in case
          continue;
        }

        handle_connection_io(conn);
      }
    }

    // try to accept a new connection if the listening fd is active
    if (((struct pollfd *)vector_get(poll_fds, 0))->revents & POLLIN) {
      accept_new_connection();
    }
  }

  return EXIT_SUCCESS;
}

static bool accept_new_connection(void) {
  // accept
  struct sockaddr_storage client_addr = {};
  socklen_t addr_size = sizeof client_addr;

  int conn_fd = accept(socket_fd, (struct sockaddr *)&client_addr, &addr_size);
  if (conn_fd == -1) {
    perror("accept()");
    return false;
  }

  // creating the struct Conn
  struct Connection conn = {
      .fd = conn_fd,
      .state = CONN_STATE_REQ,
  };

  // add the connection to the connections vector
  vector_push(connections, &conn);

  printf("client %d connected\n", conn_fd);

  return true;
}

static void handle_connection_io(struct Connection *conn) {
  if (conn->state == CONN_STATE_REQ) {
    int bytes_read =
        recv(conn->fd, conn->read_buffer, sizeof(conn->read_buffer) - 1, 0);
    if (bytes_read == -1) {
      perror("recv()");
      conn->state = CONN_STATE_END;
      return;
    } else if (bytes_read == 0) {
      printf("client %d disconnected\n", conn->fd);
      conn->state = CONN_STATE_END;
      return;
    }

    conn->read_buffer[bytes_read] = '\0';

    // this connection is ready to send a response now
    conn->state = CONN_STATE_RES;

    printf("client %d says: %s\n", conn->fd, conn->read_buffer);
  } else if (conn->state == CONN_STATE_RES) {
    char reply_start[] = "you said: ";
    memcpy(conn->write_buffer, reply_start, sizeof(reply_start));

    // concat the received message to the reply message
    strncat(conn->write_buffer, conn->read_buffer,
            sizeof(conn->write_buffer) - sizeof(reply_start) - 1);

    int bytes_sent =
        send(conn->fd, conn->write_buffer, strlen(conn->write_buffer), 0);
    if (bytes_sent == -1) {
      perror("handle_client_message(): send()");
      return;
    }

    // reset the connection state to CONN_STATE_REQ
    conn->state = CONN_STATE_REQ;
  } else {
    fputs("handle_connection_io(): invalid state\n", stderr);
    exit(EXIT_FAILURE);
  }
}

static void cleanup(void) {
  // close the socket file descriptor
  if (socket_fd != -1) {
    close(socket_fd);
  }

  // free the addrinfo linked list
  if (server_info != NULL) {
    freeaddrinfo(server_info);
  }

  // free the connections vector
  if (connections != NULL) {
    vector_free(connections);
  }

  // free the poll_fds vector
  if (poll_fds != NULL) {
    vector_free(poll_fds);
  }
}

static void handle_sigint(int sig) {
  // call exit manually because atexit() registered the cleanup function
  exit(EXIT_SUCCESS);
}
