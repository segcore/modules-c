#include "network.h"
#include <stdio.h>
#include <string.h>

int main() {
    const char* server_ip = "127.0.0.1";
    uint16_t remote_port = 42002;

    ///// Client connecting to a server
    enum NetErrorCode ec = NETERROR_NONE;
    NetHandle client;
    net_init(&client);
    while (true) {
        int timeout = 500; // -1 for infinite, 0 for no wait, >0 for milliseconds
        printf("Waiting for connect to %s:%hu...\n", server_ip, remote_port);
        ec = net_connect(&client, server_ip, remote_port, timeout);
        if (ec == NETERROR_NONE) {
            // Connected!
            printf("Connected to %s:%hu\n", server_ip, remote_port);
            break;
        } else if(ec != NETERROR_CONNECTING) {
            fprintf(stderr, "Failed to connect: %s\n", net_strerror(ec));
            return 1;
        } else {
            // Still connecting. Can come back later and re-call net_connect() on the same socket
        }
    }

    // now connected, so send/receive data
    const char* data_to_send = "client says hello!";
    ec = net_send(&client, data_to_send, strlen(data_to_send), NULL);
    if (ec != NETERROR_NONE) {
        fprintf(stderr, "Failed to sent data: %s\n", net_strerror(ec));
        return 1;
    }

    // Can check / wait for data to be readable or writable.
    // Wait for data to be readable, timeout in milliseconds
    net_wait(client.socket, WAIT_READ, 100);

    // Receive some bytes
    int timeout = 1000;
    char buf[1024];
    size_t rx_count = 0;
    ec = net_receive(&client, buf, sizeof(buf), &rx_count, timeout);
    if (ec == NETERROR_NONE) {
        printf("Received: %.*s\n", (int)rx_count, buf);
    }
    // Receive until buffer is full
    ec = net_receive_all(&client, buf, sizeof(buf), &rx_count, timeout);
    if (ec == NETERROR_NONE) {
        printf("Received: %.*s\n", (int)rx_count, buf);
    }

    // When done, close it
    net_close_handle(&client);
    return 0;
}
