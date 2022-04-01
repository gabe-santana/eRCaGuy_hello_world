/*
This file is part of eRCaGuy_hello_world:
https://github.com/ElectricRCAircraftGuy/eRCaGuy_hello_world

Edit and learn the GeeksforGeeks UDP Server-Client code, from here:
https://www.geeksforgeeks.org/udp-server-client-implementation-c/.
This is the UDP **server** code.

STATUS: DONE AND WORKS! THIS IS A GREAT DEMO!

TODO: make a super simple version of this that ignores ALL error checking, just to show how simple
and short these 5 steps are if you don't do any error checking!

To compile and run (assuming you've already `cd`ed into this dir):
1. In C:
```bash
gcc -Wall -Wextra -Werror -O3 -std=c17 socket__geeksforgeeks_udp_server_GS_edit.c -o bin/server -lm && bin/server
```
2. In C++
```bash
g++ -Wall -Wextra -Werror -O3 -std=c++17 socket__geeksforgeeks_udp_server_GS_edit.c -o bin/server && bin/server
```

------------------------------
Steps to make a UDP Server:
------------------------------
1. Create a socket with `socket()`.
2. Bind the socket object to a socket internet namespace (sin) address (which consists of a socket
internet namespace A) family, B) port, and C) IP address) via `bind()`.
3. Call `recvfrom()` to block until a message is received.
4. Process the received message and print out the sender's address info, as well as the message.
5. Call `sendto()` to send a message reply back to the sender of the message we just received.

References:

1. *****+UDP server/client: https://www.geeksforgeeks.org/udp-server-client-implementation-c/
1. *****+TCP server/client: https://www.geeksforgeeks.org/tcp-server-client-implementation-in-c/

1. https://www.binarytides.com/programming-udp-sockets-c-linux/

1. https://linux.die.net/man/7/socket
1. https://linux.die.net/man/2/socket
1. https://linux.die.net/man/2/recvfrom
1. https://man7.org/linux/man-pages/man7/ip.7.html
1. https://linux.die.net/man/7/ip
1.


What happens if you try to use **unbound sockets**!?
Ans: https://linux.die.net/man/7/ip
> When a process wants to receive new incoming packets or connections, it should bind a socket to a
  local interface address using bind(2). In this case, only one IP socket may be bound to any given
  local (address, port) pair. When INADDR_ANY is specified in the bind call, the socket will be
  bound to all local interfaces. When listen(2) is called on an unbound socket, the socket is
  automatically bound to a random free port with the local address set to INADDR_ANY. When connect
  (2) is called on an unbound socket, the socket is automatically bound to a random free port or to
  a usable shared port with the local address set to INADDR_ANY.


*/

// Server-side implementation of UDP server-client model

// local includes
// None

// Linux Includes
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

// C includes
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>  // `strerror()`


// See: https://linux.die.net/man/7/ip
// AF = "Address Family"
// INET = "Internet"
// AF_INET = IPv4 internet protocols
// AF_INET6 = IPv6 internet protocols; see: https://linux.die.net/man/2/socket
// DGRAM = "Datagram" (UDP)
#define SOCKET_TYPE_TCP              AF_INET, SOCK_STREAM, 0
#define SOCKET_TYPE_UDP              AF_INET, SOCK_DGRAM, 0
#define SOCKET_TYPE_RAW(protocol)    AF_INET, SOCK_RAW, (protocol)
// Usage examples:
// ```c
// int socket_tcp = socket(SOCKET_TYPE_TCP);
// int socket_udp = socket(SOCKET_TYPE_UDP);
// // See also: https://www.binarytides.com/raw-sockets-c-code-linux/
// int socket_raw = socket(SOCKET_TYPE_RAW(IPPROTO_RAW));
// ```c

#define MAX_RECEIVE_BUFFER_SIZE 4096  // in bytes

static const uint16_t PORT = 20000;
_Static_assert(sizeof(uint16_t) == sizeof(unsigned short),
    "`htons()` expects an unsigned short, and the PORT is uint16_t, so let's ensure they match "
    "for this system or else you'll have to replace `htons()` with a different function "
    "call.\n");


int main()
{
    int flags;

    printf("STARTING UDP SERVER:\n");

    // =============================================================================================
    printf("1. Create a socket object and obtain a file descriptor to it.\n");
    // =============================================================================================
    // See:
    // 1. https://linux.die.net/man/2/socket
    // 1. https://man7.org/linux/man-pages/man2/socket.2.html
    int socket_fd = socket(SOCKET_TYPE_UDP);
    if (socket_fd == -1)
    {
        printf("Failed to create socket. errno = %i: %s\n", errno, strerror(errno));
        goto cleanup;
    }

    // Internet namespace socket addresses
    // sockaddr_in = "socket address internet namespace"
    // See:
    // 1. GNU LibC one-page user manual:
    //    https://www.gnu.org/software/libc/manual/html_mono/libc.html#index-struct-sockaddr_005fin
    // 1. https://linux.die.net/man/7/ip
    struct sockaddr_in addr_server;
    struct sockaddr_in addr_client;
    // It's a good idea to memset the addresses to zero, so that all fields, including hidden ones
    // or padding ones, are zero as well. See: https://stackoverflow.com/a/36088131/4561887.
    memset(&addr_server, 0, sizeof(addr_server));
    memset(&addr_client, 0, sizeof(addr_client));

    // Fill in the server address information.
    // sin = "socket internet namespace"
    addr_server.sin_family = AF_INET;  // IPv4
    // NB: `htons()` converts "host to network" byte order for unsigned 's'hort types
    // (usually uint16_t), since `sin_port` must be in **network byte order**! Since the port is
    // always a 2-byte uint16_t number, use `htons()` to convert it.
    // See: https://linux.die.net/man/3/htons
    addr_server.sin_port = htons(PORT);
    // `INADDR_ANY` means "accept any incoming address". See:
    // https://www.gnu.org/software/libc/manual/html_mono/libc.html#index-INADDR_005fANY
    // To make your own address from an IPv4 string such as "127.0.0.1", use the "internet ascii to
    // network" function, `inet_aton()`, here: https://linux.die.net/man/3/inet_aton
    addr_server.sin_addr.s_addr = INADDR_ANY;

    // =============================================================================================
    printf("2. Bind the socket object with the server address specified above so that it "
           "can be used.\n");
    // =============================================================================================
    // See:
    // 1. https://linux.die.net/man/7/ip - this link also explains what happens when you try to use
    //    "unbound" sockets!
    // 1. https://linux.die.net/man/2/bind
    int retcode = bind(socket_fd, (const struct sockaddr *)&addr_server, sizeof(addr_server));
    if (retcode == -1)
    {
        printf("Failed to bind socket. errno = %i: %s\n", errno, strerror(errno));
        goto cleanup;
    }

    // =============================================================================================
    printf("3. Block until a message is received.\n");
    // =============================================================================================
    // See:
    // 1. *****https://linux.die.net/man/2/recv - NB: receive calls are all **blocking** by default
    //    but can be made **nonblocking** by passing flag `MSG_DONTWAIT` to the calls!:
    //    "If no messages are available at the socket, the receive calls wait for a message to
    //    arrive".
    // 1. https://man7.org/linux/man-pages/man2/recv.2.html
    // 1. https://www.cs.cmu.edu/~srini/15-441/F01.full/www/assignments/P2/htmlsim_split/node12.html
    char receive_buf[MAX_RECEIVE_BUFFER_SIZE];
    // This is an input/output ("value-result") argument to the `recvfrom()` call below. See the
    // documentation links above. That means we must first set it to the length of the address,
    // but then we must read it after the call to check for address length errors as well, because
    // the function will write to it!
    socklen_t addr_len = sizeof(addr_client);
    ssize_t num_bytes_received = recvfrom(socket_fd, receive_buf, sizeof(receive_buf), MSG_WAITALL,
        (struct sockaddr *)&addr_client, &addr_len);
    if (num_bytes_received == -1)
    {
        printf("Failed to receive data. errno = %i: %s\n", errno, strerror(errno));
        goto cleanup;
    }
    else if (num_bytes_received == 0)
    {
        printf("No bytes received. The sender has performed an orderly shutdown.\n");
    }

    if (addr_len > sizeof(addr_client))
    {
        printf("Error: the `addr_client` address buffer provided to the receive call was too "
               "small, and therefore the address written into it was truncated. Actual address "
               "size provided to the function was %zu bytes, but %u bytes were needed.\n",
               sizeof(addr_client), addr_len);
    }
    else if (addr_len < sizeof(addr_client))
    {
        printf("Note: the `addr_client` address buffer provided to the receive call was bigger "
               "than necessary. Actual address size provided to the function was %zu bytes, "
               "but only %u bytes were needed.\n", sizeof(addr_client), addr_len);
    }

    // =============================================================================================
    printf("4. Process the received message, & print out address info. of the sender, followed by "
           "the message!\n");
    // =============================================================================================

    // A. get and print sender address information

    // Socket internet namespace name
    const char* sender_sin_family_name = "unknown";
    switch (addr_client.sin_family)
    {
    case AF_INET:
        sender_sin_family_name = "AF_INET (IPv4 address)";
        break;
    case AF_INET6:
        sender_sin_family_name = "AF_INET6 (IPv6 address)";
    }
    // Use `ntohs()` ("network to host for short type") to convert the port number from network
    // (Big-endian) to host (Little-endian) byte order.
    uint16_t sender_port = ntohs(addr_client.sin_port);
    // See:
    // 1. https://linux.die.net/man/3/inet_aton
    // 1. https://linux.die.net/man/7/ip - for the `struct sockaddr_in` and `struct in_addr` struct
    //    definitions.
    const char* sender_ip_addr = inet_ntoa(addr_client.sin_addr);

    printf("Sender (client) address information:\n"
           "  socket internet namespace (sin) family name = %s\n"
           "  port                                        = %u\n"
           "  IP address                                  = %s\n",
           sender_sin_family_name, sender_port, sender_ip_addr);

    // B. print the message received from the sender

    printf("Msg received from sender (client) (%zi bytes):\n  %s\n",
        num_bytes_received, receive_buf);

    // =============================================================================================
    printf("5. Send a response back to the sender of the message we just received.\n");
    // =============================================================================================

    const char msg_to_send[] = "Hello from server.";
    // See: https://linux.die.net/man/2/sendto
    //
    // If you're wondering about when to use the `MSG_CONFIRM` flag, essentially, all it does is
    // tell the underlying ARP network layer to NOT periodically verify the MAC of the recipient
    // IP, because we are confident the IP we are sending to is the device we think it is, since
    // this message are are sending is in **direct response to** a message we just **received**
    // from them! In other words, if in doubt, leave OUT the `MSG_CONFIRM` flag. Put it in ONLY if
    // the message we are sending is a direct response to a message received, and therefore we want
    // to increase the network efficiency a tiny bit by NOT verifying the MAC again periodically,
    // at the risk that the destination MAC could change and be wrong and no loner match the IP
    // address for the device we think we are sending this message to!
    // - See my answer here: https://stackoverflow.com/a/71712033/4561887

    // flags = MSG_CONFIRM;
    flags = 0;
    ssize_t num_bytes_sent = sendto(socket_fd, msg_to_send, sizeof(msg_to_send), flags,
        (const struct sockaddr *)&addr_client, sizeof(addr_client));
    if (num_bytes_sent == -1)
    {
        printf("Failed to send to client. errno = %i: %s\n", errno, strerror(errno));
        goto cleanup;
    }

    printf("Done! This msg was just sent to the client:\n"
           "  %s\n", msg_to_send);


cleanup:
    if (socket_fd != -1)
    {
        close(socket_fd);
    }

    return 0;
}

/*
SAMPLE OUTPUT:

In C:

    eRCaGuy_hello_world/c$ gcc -Wall -Wextra -Werror -O3 -std=c17 socket__geeksforgeeks_udp_server_GS_edit.c -o bin/server -lm && bin/server
    STARTING UDP SERVER:
    1. Create a socket object and obtain a file descriptor to it.
    2. Bind the socket object with the server address specified above so that it can be used.
    3. Block until a message is received.
    4. Process the received message, & print out address info. of the sender, followed by the message!
    Sender address information:
      socket internet namespace (sin) family name = AF_INET (IPv4 address)
      port                                        = 55862
      IP address                                  = 127.0.0.1
    Msg received from sender (client) (18 bytes):
      Hello from client.
    5. Send a response back to the sender of the message we just received.
    Done! This msg was just sent to the client:
      Hello from server.


OR, in C++:




*/