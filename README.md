# TCP Server in C

This is a basic single-threaded TCP server with an event loop using the poll system call, written in C, without any third-party library. The server listens on a port and echoes back received data. I've also written a simple client to test the server.

For details on the [resources](#resources) utilized in this project, please refer to the section provided below.

## How to run

1. Clone the repository

2. Compile client and server programs (don't worry, there's a Makefile to do this for you)

   ```bash
   make
   ```

3. Run the server (it will run on port 3001)

   ```bash
   ./bin/server
   ```

4. Run the client in another terminal

   ```bash
   ./bin/client
   ```

5. Type something in the client terminal and press enter. You should see the server echoing back what you typed.

6. Ctrl+C to stop the server and client.

## Resources

- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/html/multi/index.html)
- [The Event Loop Implementation](https://build-your-own.org/redis/06_event_loop_impl)
- [Johannes 4GNU_Linux's YouTube video](https://www.youtube.com/watch?v=O-yMs3T0APU)
