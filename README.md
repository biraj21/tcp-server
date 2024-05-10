# TCP Server in C

This is a simple TCP server written from scratch in C. It is a simple server that listens on a port and echos back whatever it receives.

I've followed the tutorial at [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/html/multi/index.html) to gain an understanding of sockets before writing this server.

Note that currently it only supports one client. Will probably learn and implement an event loop kind of thing to support multiple clients.

## How to run

1. Clone the repository

2. Compile client and server programs (don't worry, there's a Makefile to do this for you)

   ```bash
   make
   ```

3. Run the server (it will run on port 3000)

   ```bash
   ./bin/server
   ```

4. Run the client in another terminal

   ```bash
   ./bin/client
   ```

5. Type something in the client terminal and press enter. You should see the server echoing back what you typed.

6. Ctrl+C to stop the server and client.
