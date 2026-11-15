/*
 * network.h - cross-platform network abstraction layer.
 * Currently supports TCP only. I will add UDP if/when I need it.
 *
 * https://github.com/segcore/modules-c
 *
 * Licence: see end of file.

 Sample usage of the API flow:
    #include "network.h"

    int tcp_client_connecting_to_a_server() {
        NetHandle client;
        net_init(&client);
        ec = net_connect(&client, server_ip, remote_port, timeout);
        if (ec == NETERROR_NONE) {
            // Connected!
        } else if(ec != NETERROR_CONNECTING) {
            // Failed to connect
        } else {
            // Still connecting. Can come back later and re-call net_connect() on the same socket
        }

        net_send(&client, data_to_send, strlen(data_to_send), NULL);
        net_wait(client.socket, WAIT_READ, 100);
        net_receive(&client, buf, sizeof(buf), &rx_count, timeout);
        net_receive_all(&client, buf, sizeof(buf), &rx_count, timeout);
        net_close_handle(&client);
    }


    int tcp_server_accepting_from_clients() {
        NetListener server;
        net_init_listener(&server);
        net_listen(&server, local_ip, local_port);
        while (true) {
            NetHandle new_client;
            ec = net_accept(&server, &new_client, timeout);
            if (ec == NETERROR_NONE) {
                // Send/receive with the new client
                net_receive(&new_client, buf, sizeof(buf), &rx_once, 3000);
                net_send(&new_client, buf, tx_count, NULL);
                net_close_handle(&new_client);
            } else if(ec == NETERROR_ACCEPT_WAITING) {
                // Wait, doing something else or wait on the socket
                net_wait(server.listener, WAIT_ANY, 10000);
            } else {
                fprintf(stderr, "Accept failed. Probably fatal. Stopping. Error: %s\n", net_strerror(ec));
                return 1;
            }
        }
        net_close_listener(&server);
    }
 */

#ifndef SEGCORE_NETWORK_H
#define SEGCORE_NETWORK_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Flag combinations used when waiting for data
#ifndef WAIT_READ
    #define WAIT_READ  0x1  //!< Wait for reading to become available
    #define WAIT_WRITE 0x2  //!< Wait for writing to become available
    #define WAIT_ANY   (WAIT_READ | WAIT_WRITE) // !< Wait for reading or writing to become available
#endif

#ifdef _WIN32
    typedef size_t Socket;
    #ifndef INVALID_SOCKET
        #define INVALID_SOCKET (Socket)(~0)
    #endif
#else
    typedef int Socket;
    #ifndef INVALID_SOCKET
        #define INVALID_SOCKET -1
    #endif
#endif

enum ConnectionState
{
    CONSTATE_NONE,
    CONSTATE_CONNECTING,
    CONSTATE_CONNECTED,
};

enum NetErrorCode {
    NETERROR_NONE = 0,                    //!< No error
    NETERROR_INVALID_REMOTE_ADDR,         //!< Invalid remote address
    NETERROR_INVALID_LOCAL_ADDR,          //!< Invalid local address
    NETERROR_CONNECTION_REFUSED,          //!< Connection refused
    NETERROR_NETWORK_UNREACHABLE,         //!< Network unreachable
    NETERROR_CONNECTING,                  //!< Connecting...
    NETERROR_CONNECT_TIMEOUT,             //!< Connect timeout
    NETERROR_CONNECT_FAILED,              //!< Connect failed
    NETERROR_ALREADY_LISTENING,           //!< Already listening
    NETERROR_ACCEPT_WAITING,              //!< Accept waiting
    NETERROR_ACCEPT_FAILED,               //!< Accept failed
    NETERROR_SOCKET_CREATE,               //!< Socket create error
    NETERROR_SOCKET_BIND,                 //!< Socket bind error
    NETERROR_SOCKET_LISTEN,               //!< Socket listen error
    NETERROR_SOCKET_OPTIONS,              //!< Socket options error
    NETERROR_NOT_CONNECTED,               //!< Not connected
    NETERROR_NETWORK_RESET,               //!< Peer closed the connection
    NETERROR_NETWORK_SEND_NOT_CONNECTED,  //!< Cannot send; not connected.
    NETERROR_NETWORK_SEND_FAIL,           //!< Send failed
    NETERROR_SEND_PARTIAL,                //!< Partial data sent
    NETERROR_END_OF_DATA,                 //!< End of data
    NETERROR_NETWORK_RECEIVE,             //!< Network receive error
    NETERROR_WOULD_BLOCK,                 //!< Action would block; returning early
    NETERROR_BUFFER_FULL,                 //!< Buffer is full
    NETERROR_BUFFER_TOO_SMALL,            //!< Buffer is too small to fit the payload
    NETERROR_INVALID_MESSAGE,             //!< Invalid message
};

// Forward declaration
struct addrinfo;

typedef struct {
    Socket socket;
    enum ConnectionState state;
    struct addrinfo* base_addr_list;
    struct addrinfo* addr_list;
} NetHandle;

typedef struct {
    Socket listener;
} NetListener;


const char* net_strerror(enum NetErrorCode net_error);

void net_init(NetHandle* net);
enum NetErrorCode net_connect(NetHandle* net, const char* remote_name, uint16_t remote_port, int timeout_ms);

void net_init_listener(NetListener* listener);
enum NetErrorCode net_listen(NetListener* listener, const char* local_addr, uint16_t local_port);
enum NetErrorCode net_accept(NetListener* listener, NetHandle* out_accepted, int timeout_ms);

enum NetErrorCode net_send(NetHandle* net, const void* data, size_t size, size_t* bytes_sent);
enum NetErrorCode net_receive(NetHandle* net, void* data, size_t buffer_size, size_t* bytes_received, int timeout_ms);
enum NetErrorCode net_receive_all(NetHandle* net, void* data, size_t buffer_size, size_t* bytes_received, int timeout_ms);

// Wait for an event.
// wait_type: Bitwise flags on what to wait for, see WAIT_READ and WAIT_WRITE.
// timeout_ms: <0 wait forever, 0 no wait, >0 wait max that many milliseconds.
// returns: true if given action is ready, or an error on the socket. false otherwise.
bool net_wait(Socket sock, int wait_type, int timeout_ms);
void net_close_socket(Socket sock);
void net_close_handle(NetHandle* net);
void net_close_listener(NetListener* listener);

#endif

/*
------------------------------------------------------------------------------
This software is available under 2 licenses -- choose whichever you prefer.
------------------------------------------------------------------------------
ALTERNATIVE A - MIT License

Copyright (c) 2026 Nicholas Rixson

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
------------------------------------------------------------------------------
ALTERNATIVE B - Public Domain (www.unlicense.org)
This is free and unencumbered software released into the public domain.
Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
software, either in source code form or as a compiled binary, for any purpose,
commercial or non-commercial, and by any means.

In jurisdictions that recognize copyright laws, the author or authors of this
software dedicate any and all copyright interest in the software to the public
domain. We make this dedication for the benefit of the public at large and to
the detriment of our heirs and successors. We intend this dedication to be an
overt act of relinquishment in perpetuity of all present and future rights to
this software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------
*/
