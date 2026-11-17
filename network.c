#include "network.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#if defined(_WIN32)

    #if defined(__MINGW32__)
        #include <sys/types.h>
        #include <unistd.h>
        #include <ws2tcpip.h>
    #else
        #include <ws2tcpip.h>
        #pragma comment(lib, "ws2_32.lib")

        typedef int ssize_t;    //!< Not really correct, but recv() returns int on Windows
    #endif

    #define close   closesocket
    #define poll    WSAPoll

    #define FLAGS_SOCK  0
    #define FLAGS_SEND  0
    #define FLAGS_RECV  0
    #define MYPOLL_READ   POLLIN

    #define SHUT_RD    SD_RECEIVE
    #define SHUT_WR    SD_SEND
    #define SHUT_RDWR  SD_BOTH

    static void socket_setup(Socket sock)
    {
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&(DWORD){0}, sizeof(DWORD));
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&(DWORD){0}, sizeof(DWORD));
        ioctlsocket(sock, FIONBIO, &(u_long){1});
    }

    #include <sysinfoapi.h> // GetTickCount64()
    static double monotonic_time(void)
    {
        // This isn't very precise (e.g. 15ms increments)
        return GetTickCount64() / 1000.0;
    }
#else

    #include <errno.h>
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <unistd.h>
    #include <netdb.h>
    #include <poll.h>

    #define FLAGS_SOCK  SOCK_NONBLOCK | SOCK_CLOEXEC
    #define FLAGS_SEND  MSG_DONTWAIT | MSG_NOSIGNAL
    #define FLAGS_RECV  MSG_DONTWAIT
    #define MYPOLL_READ   POLLIN | POLLPRI

    static inline void socket_setup(Socket sock) { (void)sock; } // nothing, with type checking

    #include <time.h>
    static double monotonic_time(void)
    {
        struct timespec spec = {0};
        clock_gettime(CLOCK_MONOTONIC, &spec);
        double t = spec.tv_sec + spec.tv_nsec / 1e9;
        return t;
    }

#endif


// Intentionally not static, in case someone outside wants to know what we've done
int net_globals_state = 0;

void net_globals_startup(bool do_initialisation)
{
    (void)do_initialisation; // silence unused warning
#ifdef _WIN32
    if (net_globals_state == 0) {
        net_globals_state |= 1;
        if (do_initialisation) {
            net_globals_state |= 2;
            WSADATA idontcare;
            int ret = WSAStartup(0x202, &idontcare);
            if (ret != 0)
            {
                fprintf(stderr, "ERROR: WSAStartup failed with error code %d\n", ret);
            }
        }
    }
#endif
}

void net_globals_shutdown()
{
#ifdef _WIN32
    if (net_globals_state & 2) {
        int ret = WSACleanup();
        if (ret != 0) {
            fprintf(stderr, "WSACleanup() failed with error code %d\n", WSAGetLastError());
        }
    }
    net_globals_state = 0;
#endif
}

void net_init(NetHandle* net)
{
    net_globals_startup(true);
    memset(net, 0, sizeof *net);
    net->socket = INVALID_SOCKET;
}


void net_init_listener(NetListener* listener)
{
    net_globals_startup(true);
    memset(listener, 0, sizeof *listener);
    listener->listener = INVALID_SOCKET;
}


static enum NetErrorCode net_connect_once_internal(NetHandle* net, const char* remote_name, uint16_t remote_port)
{
    enum NetErrorCode ec = NETERROR_NONE;
    // net_connect_once_internal may be called periodically to know when it is connected successfully
    // So the function must be re-entered while the socket is known and valid

    // First entry; generate the name lookups
    if (net->socket == INVALID_SOCKET && net->addr_list == NULL)
    {
        assert(net->base_addr_list == NULL);

        struct addrinfo* base_addr_info = NULL;
        struct addrinfo hints;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = 0;
        hints.ai_flags = AI_ADDRCONFIG | AI_V4MAPPED;
        char port_string[6];
        snprintf(port_string, sizeof(port_string), "%hu", remote_port);
        int ret = getaddrinfo(remote_name, port_string, &hints, &base_addr_info);
        if (ret == 0)
        {
            // success
            net->base_addr_list = base_addr_info;
            net->addr_list = base_addr_info;
        }
        else
        {
            ec = NETERROR_INVALID_REMOTE_ADDR;
        }
    }

    if (net->addr_list != NULL)
    {
        struct addrinfo* addr = net->addr_list;

        Socket sock = net->socket;
        bool connected = false;
        for(; addr != NULL; addr = addr->ai_next)
        {
            bool use_socket = false;
            if (sock == INVALID_SOCKET)
            {
                sock = socket(addr->ai_family, addr->ai_socktype | FLAGS_SOCK, addr->ai_protocol);
                if (sock == INVALID_SOCKET)
                {
                    continue;
                }
                else
                {
                    socket_setup(sock);
                }
            }

            // TODO: If the socket has already been called with connect(), man(2) connect suggests
            // we should use getsockopt(2) to read SO_ERROR at SOL_SOCKET to determine if it is connected.
            // But in practice it seems that this connect() method works.
            net->state = CONSTATE_CONNECTING;
            if (connect(sock, addr->ai_addr, addr->ai_addrlen) != -1)
            {
                // Connected successfully!
                connected = true;
                break;
            }

#if defined _WIN32
            switch(WSAGetLastError()) {
                case WSAEISCONN: connected = true; break;
                case WSAEWOULDBLOCK: use_socket = true; break;
                case WSAEADDRINUSE: ec = NETERROR_SOCKET_BIND; break;
                case WSAEALREADY: /* connect already active */ ec = NETERROR_CONNECTING; break;
                case WSAECONNREFUSED: ec = NETERROR_CONNECTION_REFUSED; break;
                case WSAETIMEDOUT: ec = NETERROR_CONNECT_TIMEOUT; break;
                default:
                    ec = NETERROR_CONNECT_FAILED;
                    break;
            }
#else
            switch(errno) {
                case EISCONN:
                    // Already connected
                    connected = true;
                    break;
                case EINPROGRESS:
                case EALREADY:
                    // Non-blocking and has not yet completed
                    use_socket = true;
                    break;
                case ECONNREFUSED: ec = NETERROR_CONNECTION_REFUSED; break;
                case ENETUNREACH: ec = NETERROR_NETWORK_UNREACHABLE; break;
                case ETIMEDOUT: ec = NETERROR_CONNECT_TIMEOUT; break;
                default:
                    ec = NETERROR_CONNECT_FAILED;
                    break;
            }
#endif

            if (connected || use_socket)
            {
                break;
            }

            close(sock);
            sock = INVALID_SOCKET;
        }
        net->socket = sock;

        if (connected)
        {
            assert(net->socket != INVALID_SOCKET);
            freeaddrinfo(net->base_addr_list);
            net->base_addr_list = NULL;
            net->addr_list = NULL;
            net->state = CONSTATE_CONNECTED;
            ec = NETERROR_NONE;
        }
        else
        {
            net->addr_list = addr;
            if (addr == NULL)
            {
                assert(sock == INVALID_SOCKET);
                // No valid addresses
                freeaddrinfo(net->base_addr_list);
                net->base_addr_list = NULL;
                if (ec == NETERROR_NONE)
                {
                    ec = NETERROR_CONNECT_FAILED;
                }
                net->state = CONSTATE_NONE;
            }
            else
            {
                assert(sock != INVALID_SOCKET);
                ec = NETERROR_CONNECTING;
            }
        }
    }

    return ec;
}


enum NetErrorCode net_connect(NetHandle* net, const char* remote_name, uint16_t remote_port, int timeout_ms)
{
    enum NetErrorCode ec = NETERROR_NONE;
    ec = net_connect_once_internal(net, remote_name, remote_port);
    if (ec == NETERROR_CONNECTING) {
        if (net_wait(net->socket, WAIT_WRITE, timeout_ms)) {
            ec = net_connect_once_internal(net, remote_name, remote_port);
        }
    }
    return ec;
}


enum NetErrorCode net_listen(NetListener* listener, const char* local_addr, uint16_t local_port)
{
    enum NetErrorCode ec = NETERROR_NONE;

    struct addrinfo* base_addr_info = NULL;
    if (listener->listener == INVALID_SOCKET)
    {
        struct addrinfo hints;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = 0;
        hints.ai_flags = AI_ADDRCONFIG | AI_V4MAPPED | AI_PASSIVE | AI_NUMERICSERV;
        char port_string[6];
        snprintf(port_string, sizeof(port_string), "%hu", local_port);
        int ret = getaddrinfo(local_addr, port_string, &hints, &base_addr_info);
        if (ret != 0) {
            ec = NETERROR_INVALID_LOCAL_ADDR;
        }
    }
    else
    {
        ec = NETERROR_ALREADY_LISTENING;
    }

    if (base_addr_info != NULL)
    {
        Socket sock = INVALID_SOCKET;

        // Try all bind addresses until one succeeds
        for(struct addrinfo* addr = base_addr_info; addr != NULL; addr = addr->ai_next)
        {
            sock = socket(addr->ai_family, addr->ai_socktype | FLAGS_SOCK, addr->ai_protocol);
            bool use_socket = true;
            if (sock == INVALID_SOCKET)
            {
                ec = NETERROR_SOCKET_CREATE;
                continue;
            }
            socket_setup(sock);

            if (use_socket && setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (void*)&(int){1}, sizeof(int)) < 0)
            {
                use_socket = false;
                ec = NETERROR_SOCKET_OPTIONS;
            }

            if (use_socket && bind(sock, addr->ai_addr, addr->ai_addrlen) == -1)
            {
                use_socket = false;
                ec = NETERROR_SOCKET_BIND;
            }

            if (use_socket && listen(sock, 32) == -1)
            {
                use_socket = false;
                ec = NETERROR_SOCKET_LISTEN;
            }

            if (use_socket)
            {
                break;
            }

            close(sock);
            sock = INVALID_SOCKET;
        }
        listener->listener = sock;

        freeaddrinfo(base_addr_info);
    }

    return ec;
}


enum NetErrorCode net_accept(NetListener* listener, NetHandle* out_accepted, int timeout_ms)
{
    enum NetErrorCode ec = NETERROR_ACCEPT_WAITING;

    struct sockaddr remote_addr;
    socklen_t remote_addr_len = sizeof(remote_addr);

    if (net_wait(listener->listener, WAIT_READ, timeout_ms)) {
        int accepted = accept(listener->listener, &remote_addr, &remote_addr_len);
        if (accepted >= 0)
        {
            // Accept successful
            net_init(out_accepted);
            out_accepted->socket = accepted;
            out_accepted->state = CONSTATE_CONNECTED;
            ec = NETERROR_NONE;
        } else {
#ifdef _WIN32
            switch(WSAGetLastError()) {
                case WSAECONNRESET: ec = NETERROR_ACCEPT_WAITING; break; // An incoming connection was indicated, but was subsequently terminated by the remote peer prior to accepting the call.
                case WSAEWOULDBLOCK: ec = NETERROR_ACCEPT_WAITING; break;
                default: ec = NETERROR_ACCEPT_FAILED; break;
            }
#else
            switch(errno) {
                case EAGAIN:
#if EAGAIN != EWOULDBLOCK
                case EWOULDBLOCK:
#endif
                    ec = NETERROR_ACCEPT_WAITING;
                    break;
                default:
                    ec = NETERROR_ACCEPT_FAILED;
                    break;
            }
#endif
        }
    }

    return ec;
}


enum NetErrorCode net_send(NetHandle* net, const void* data, size_t size, size_t* bytes_sent)
{
    if (net->state == CONSTATE_NONE) return NETERROR_NOT_CONNECTED;

    enum NetErrorCode ec = NETERROR_NONE;
    ssize_t ret;
retry:
    ret = send(net->socket, data, size, FLAGS_SEND);
    if (ret > 0) {
        size_t uret = (size_t)ret;
        if (bytes_sent != NULL) {
            *bytes_sent = uret;
        }

        // Sending partial message is probably worth quitting over
        // If not, the higher layer can decide for itself
        if (uret != size) {
            ec = NETERROR_SEND_PARTIAL;
        }
    } else {
#if _WIN32
        switch(WSAGetLastError()) {
            case WSAEWOULDBLOCK:
                // Unable to send, for now...
                *bytes_sent = 0;
                break;
            case WSAENOTCONN: ec = NETERROR_NETWORK_SEND_NOT_CONNECTED; break;
            case WSAESHUTDOWN: ec = NETERROR_NETWORK_SEND_NOT_CONNECTED; break;
            case WSAECONNRESET: ec = NETERROR_NETWORK_RESET; break;
            default: ec = NETERROR_NETWORK_SEND_FAIL; break;
        }
#else
        switch(errno) {
            case EAGAIN:
#if EAGAIN != EWOULDBLOCK
            case EWOULDBLOCK:
#endif
                // Unable to send, for now...
                *bytes_sent = 0;
                break;
            case ECONNRESET: ec = NETERROR_NETWORK_RESET; break;
            case EINTR: // We wanna try again. Who says gotos are bad?
                goto retry;
            case EPIPE: ec = NETERROR_NETWORK_SEND_NOT_CONNECTED; break;
            default: ec = NETERROR_NETWORK_SEND_FAIL; break;
        }
#endif
    }

    return ec;
}


enum NetErrorCode net_receive(NetHandle* net, void* data, size_t buffer_size, size_t* bytes_received, int timeout_ms)
{
    if (net->state == CONSTATE_NONE) return NETERROR_NOT_CONNECTED;
    size_t received_count = 0;

    enum NetErrorCode ec = NETERROR_WOULD_BLOCK;
    if (timeout_ms == 0 || net_wait(net->socket, WAIT_READ, timeout_ms)) {
        ssize_t ret;
retry:
        ret = recv(net->socket, data, buffer_size, FLAGS_RECV);
        if (ret > 0) {
            received_count = (size_t)ret;
            ec = NETERROR_NONE;
        } else if (ret == 0) {
            if (buffer_size == 0) {
                ec = NETERROR_BUFFER_FULL;
            } else {
                ec = NETERROR_END_OF_DATA;
            }
        } else {
#if defined _WIN32
            switch(WSAGetLastError()) {
                case WSAENOTCONN: ec = NETERROR_NOT_CONNECTED; break;
                case WSAESHUTDOWN: ec = NETERROR_END_OF_DATA; break;
                case WSAEWOULDBLOCK: break;
                case WSAECONNRESET: ec = NETERROR_NETWORK_RESET; break;
                default: ec = NETERROR_NETWORK_RECEIVE; break;
            }
#else
            switch(errno) {
                case EAGAIN:
#if EAGAIN != EWOULDBLOCK
                case EWOULDBLOCK:
#endif
                    break;
                case EINTR: goto retry;
                default: ec = NETERROR_NETWORK_RECEIVE; break;
            }
#endif
        }
    }

    if (bytes_received != NULL) {
        *bytes_received = received_count;
    }

    return ec;
}

enum NetErrorCode net_receive_all(NetHandle* net, void* data, size_t buffer_size, size_t* bytes_received, int timeout_ms)
{
    enum NetErrorCode ec = NETERROR_WOULD_BLOCK;
    double start = monotonic_time();
    size_t rx_total = 0;
    size_t rx_once = 0;
    int next_timeout = timeout_ms;
    do {
        while (rx_total < buffer_size) {
            enum NetErrorCode ec_once = net_receive(net, data + rx_total, buffer_size - rx_total, &rx_once, next_timeout);
            if (ec_once == NETERROR_NONE) {
                rx_total += rx_once;
            } else {
                ec = ec_once;
                break;
            }
        }

        if (rx_total == buffer_size) {
            ec = NETERROR_NONE;
            break;
        } else if (ec != NETERROR_NONE) {
            // Failed. Stop now.
            // But must indicate partial data has been received, if it was.
            // The next call to receive will give the more detailed error
            if (rx_total > 0) {
                ec = NETERROR_NETWORK_RECEIVE_PARTIAL;
            }
            break;
        } else {
            if (timeout_ms > 0) {
                double now = monotonic_time();
                int time_used_up = (int)(1000.0 * (now - start));
                if (time_used_up >= timeout_ms) {
                    next_timeout = 0;
                } else {
                    next_timeout = timeout_ms - time_used_up;
                }
            }
        }
    } while (next_timeout != 0);

    if (bytes_received) *bytes_received = rx_total;

    return ec;
}


bool net_wait(Socket sock, int wait_type, int timeout_ms)
{
    int events = 0;
    if (wait_type & WAIT_READ) {
        events |= MYPOLL_READ;
    }
    if (wait_type & WAIT_WRITE) {
        events |= POLLOUT;
    }

    struct pollfd fds[1];
    fds[0].fd = sock;
    fds[0].events = events;
    int ret = poll(fds, 1, timeout_ms);
    return ret != 0;
}


bool net_shutdown(NetHandle* net, int shutdown_type)
{
    int ret = -1;
    net->shutdown |= shutdown_type; // Stores what is shutdown
    if ((shutdown_type & NET_SHUTDOWN_ALL) == NET_SHUTDOWN_ALL) {
        ret = shutdown(net->socket, SHUT_RDWR);
    } else if(shutdown_type & NET_SHUTDOWN_READ) {
        ret = shutdown(net->socket, SHUT_RD);
    } else if(shutdown_type & NET_SHUTDOWN_WRITE) {
        ret = shutdown(net->socket, SHUT_WR);
    }
    bool ok = ret == 0;
    return ok;
}


void net_close_socket(Socket sock)
{
    if (sock != INVALID_SOCKET) {
        close(sock);
    }
}


void net_close_handle(NetHandle* net)
{
    net_close_socket(net->socket);
    net->socket = INVALID_SOCKET;
    net->state = CONSTATE_NONE;
    if (net->base_addr_list != NULL) {
        freeaddrinfo(net->base_addr_list);
        net->base_addr_list = NULL;
        net->addr_list = NULL;
    }
}

void net_close_listener(NetListener* listener)
{
    net_close_socket(listener->listener);
    listener->listener = INVALID_SOCKET;
}

const char* net_strerror(enum NetErrorCode net_error)
{
    switch(net_error) {
        case NETERROR_NONE:                        return "No error";
        case NETERROR_INVALID_REMOTE_ADDR:         return "Invalid remote address";
        case NETERROR_INVALID_LOCAL_ADDR:          return "Invalid local address";
        case NETERROR_CONNECTION_REFUSED:          return "Connection refused";
        case NETERROR_NETWORK_UNREACHABLE:         return "Network unreachable";
        case NETERROR_CONNECTING:                  return "Connecting...";
        case NETERROR_CONNECT_TIMEOUT:             return "Connect timeout";
        case NETERROR_CONNECT_FAILED:              return "Connect failed";
        case NETERROR_ALREADY_LISTENING:           return "Already listening";
        case NETERROR_ACCEPT_WAITING:              return "Accept waiting";
        case NETERROR_ACCEPT_FAILED:               return "Accept failed";
        case NETERROR_SOCKET_CREATE:               return "Socket create error";
        case NETERROR_SOCKET_BIND:                 return "Socket bind error";
        case NETERROR_SOCKET_LISTEN:               return "Socket listen error";
        case NETERROR_SOCKET_OPTIONS:              return "Socket options error";
        case NETERROR_NOT_CONNECTED:               return "Not connected";
        case NETERROR_NETWORK_RESET:               return "Peer closed the connection";
        case NETERROR_NETWORK_SEND_NOT_CONNECTED:  return "Cannot send; not connected.";
        case NETERROR_NETWORK_SEND_FAIL:           return "Send failed";
        case NETERROR_SEND_PARTIAL:                return "Partial data sent";
        case NETERROR_END_OF_DATA:                 return "End of data";
        case NETERROR_NETWORK_RECEIVE:             return "Network receive error";
        case NETERROR_NETWORK_RECEIVE_PARTIAL:     return "Network receive all attempted but partial data received";
        case NETERROR_WOULD_BLOCK:                 return "Action would block; returning early";
        case NETERROR_BUFFER_FULL:                 return "Buffer is full";
        case NETERROR_BUFFER_TOO_SMALL:            return "Buffer is too small to fit the payload";
        case NETERROR_INVALID_MESSAGE:             return "Invalid message";
    }
    return "?";
}

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
