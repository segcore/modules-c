#include "network.h"
#include <stdio.h>
#include <string.h>

int main() {
    ///// Server receiving client connections
    const char* local_ip = "127.0.0.1";
    uint16_t local_port = 42002;
    enum NetErrorCode ec = NETERROR_NONE;

    NetListener server;
    net_init_listener(&server);

    // Start listening
    printf("Attempt to listen on %s:%hu...\n", local_ip, local_port);
    ec = net_listen(&server, local_ip, local_port);
    if (ec != NETERROR_NONE) {
        printf("! Failed to listen on %s:%hu - %s\n", local_ip, local_port, net_strerror(ec));
        if (ec == NETERROR_SOCKET_BIND) {
            printf("  Maybe the port is in use?\n");
        }
        return 1;
    }

    while (true) {
        int timeout = 0; // -1 for infinite, 0 for no wait, >0 for milliseconds
        NetHandle new_client;
        ec = net_accept(&server, &new_client, timeout);
        if (ec == NETERROR_NONE) {
            printf("\n* A new client has connected to us! socket %d. Waiting for their message\n", (int)new_client.socket);

            // Maybe save new_client into a list for sending more data to later.

            // This example is just a simple echo of 1x received set of bytes
            char buf[1024];
            size_t rx_once = 0;
            ec = net_receive(&new_client, buf, sizeof(buf), &rx_once, 10000);
            if (ec == NETERROR_NONE) {
                printf("<> Echoing: %.*s\n", (int)rx_once, buf);
                ec = net_send(&new_client, buf, rx_once, NULL);
                if (ec != NETERROR_NONE) {
                    printf("Failed to send some bytes back to the client\n");
                }

                // We've stopped writing, so 'do the right thing' for the TCP handshake
                // to close everything up nicely
                // (most people just close the socket, which can work too but has possible loss-of-data in transmission)
                printf("* Shutting down the socket for writing. Will wait for any final data.\n");
                net_shutdown(&new_client, NET_SHUTDOWN_WRITE);

                // To be fully nice, we're supposed to wait for the other end to acknowledge this fact
                // and read anything they may have sent to us in the mean time
                ec = net_receive_all(&new_client, buf, sizeof(buf), &rx_once, 10000);
                if (ec == NETERROR_NONE || ec == NETERROR_NETWORK_RECEIVE_PARTIAL) {
                    printf("< The client's last message: %.*s\n", (int)rx_once, buf);
                } else {
                    printf("* Final receive failed: %s\n", net_strerror(ec));
                }
            } else {
                printf("* No client data. Closing.\n");
            }
            printf("  socket %d close\n\n", (int)new_client.socket);
            net_close_handle(&new_client);
        } else if(ec == NETERROR_ACCEPT_WAITING) {
            printf("Waiting for someone to connect...\n");
            net_wait(server.listener, WAIT_ANY, 10000);
        } else {
            fprintf(stderr, "Accept failed. Probably fatal. Stopping. Error: %s\n", net_strerror(ec));
            return 1;
        }
    }

    net_close_listener(&server);

    return 0;
}
