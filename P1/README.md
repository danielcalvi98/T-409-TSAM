# Basic server and client

The client sends command to server and server sends output of set command back to client

## How to run

Use the 'make' command in the terminal to compile both the server and client, make sure to be in the correct directory.

```bash
make
```

## Usage

Run the server on a UNIX system in the terminal along with ip port:

```bash
./server <ip port>
```

Run the client on the same or other UNIX system in the terminal along with both an ip address and an ip port:

```bash
./client <ip address> <ip port>
```