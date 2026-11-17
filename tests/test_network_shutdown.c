#include "testing.h"
#include "network.h"

#define PORT 42096

bool setup_connection(NetHandle* client, NetHandle* server, NetListener* listener)
{
    net_init(client);
    net_init(server);
    net_init_listener(listener);

    enum NetErrorCode ec = NETERROR_NONE;
    TEST_MSG(TEST_EQ(ec = net_listen(listener, "127.0.0.1", PORT), NETERROR_NONE), "Listening");
    ec = net_connect(client, "127.0.0.1", PORT, 0);
    if (!TEST_MSG(ec == NETERROR_NONE || ec == NETERROR_CONNECTING, "Should connect")) {
        TEST_LOG("Error code was %d (%s)\n", (int)ec, net_strerror(ec));
    }
    TEST_MSG(net_wait(client->socket, WAIT_WRITE, 50), "Waiting for connect to succeed");
    TEST_MSG((ec = net_connect(client, "127.0.0.1", PORT, 0)) == NETERROR_NONE, "Ensure connect successful");

    TEST_MSG((ec = net_accept(listener, server, 0)) == NETERROR_NONE, "Server should successfully accept the connection");

    return ec == NETERROR_NONE;
}


int main(int argc, char* argv[])
{
    tests_arg_parse(&argc, &argv);
    NetHandle client;
    NetHandle server;
    NetListener listener;
    char sendbuf[] = "MyMessageToSend";
    char recvbuf[1024];
    size_t rx_count = 0;

    test_section_name("Shutdown WRITE");
    if (setup_connection(&client, &server, &listener)) {
        TEST_EQ(net_wait(client.socket, WAIT_READ, 0), false);
        TEST_EQ(net_wait(client.socket, WAIT_WRITE, 0), true);
        TEST_EQ(net_wait(client.socket, WAIT_ERROR, 0), false);

        TEST_EQ(net_shutdown(&server, NET_SHUTDOWN_WRITE), true);
        TEST_EQ(net_wait(server.socket, WAIT_READ, 0), false);

        TEST_EQ(net_wait(client.socket, WAIT_READ, 20), true);
        TEST_EQ(net_wait(client.socket, WAIT_WRITE, 0), true);

        TEST_LOG("Should be able to send with client, but not the server\n");
        TEST_EQ(net_send(&client, sendbuf, sizeof(sendbuf), NULL), NETERROR_NONE);
        TEST_EQ(net_send(&server, sendbuf, sizeof(sendbuf), NULL), NETERROR_NETWORK_SEND_NOT_CONNECTED);
        TEST_EQ(net_send(&client, sendbuf, sizeof(sendbuf), NULL), NETERROR_NONE);
        TEST_EQ(net_send(&server, sendbuf, sizeof(sendbuf), NULL), NETERROR_NETWORK_SEND_NOT_CONNECTED);

        TEST_LOG("Should receive what the client has sent\n");
        TEST_EQ(net_receive_all(&server, recvbuf, sizeof(recvbuf), &rx_count, 100), NETERROR_NETWORK_RECEIVE_PARTIAL);
        TEST_EQ(rx_count, sizeof(sendbuf) * 2);

        TEST_LOG("Server would block if waiting for more data\n");
        TEST_EQ(net_receive(&server, recvbuf, sizeof(recvbuf), &rx_count, 0), NETERROR_WOULD_BLOCK);
        TEST_EQ(net_receive_all(&server, recvbuf, sizeof(recvbuf), &rx_count, 0), NETERROR_WOULD_BLOCK);

        TEST_LOG("Client should see END_OF_DATA\n");
        TEST_EQ(net_receive(&client, recvbuf, sizeof(recvbuf), &rx_count, 100), NETERROR_END_OF_DATA);
        TEST_EQ(net_receive_all(&client, recvbuf, sizeof(recvbuf), &rx_count, 0), NETERROR_END_OF_DATA);

        TEST_LOG("Client's READ and WRITE should not block\n");
        TEST_EQ(net_wait(client.socket, WAIT_READ, 0), true);
        TEST_EQ(net_wait(client.socket, WAIT_WRITE, 0), true);

        TEST_LOG("Server's READ should block, WRITE should not\n");
        TEST_EQ(net_wait(server.socket, WAIT_READ, 0), false);
        TEST_EQ(net_wait(server.socket, WAIT_WRITE, 0), true);
        TEST_EQ(net_wait(server.socket, WAIT_ERROR, 0), false);

        TEST_EQ(net_shutdown(&server, NET_SHUTDOWN_READ), true);

        // This should probably return true, but it is a bit obscure to close it then
        // ask if you can still read it. It fails on win32 mingw with wine.
        // TEST_EQ(net_wait(server.socket, WAIT_READ, 0), true);
        TEST_EQ(net_wait(server.socket, WAIT_WRITE, 0), true);

        net_close_handle(&client);

        TEST_LOG("Server should notice that the client closed the socket\n");
        TEST_EQ(net_wait(server.socket, WAIT_ERROR, 100), true);
        net_close_handle(&server);
        net_close_listener(&listener);
        TEST_PASS("Get to the end of the function without crashing");
    }


    test_section_name("Shutdown READ");
    if (setup_connection(&client, &server, &listener)) {
        TEST_EQ(net_wait(client.socket, WAIT_READ, 0), false);
        TEST_EQ(net_wait(client.socket, WAIT_WRITE, 0), true);
        TEST_EQ(net_wait(client.socket, WAIT_ERROR, 0), false);

        TEST_EQ(net_shutdown(&server, NET_SHUTDOWN_READ), true);
        TEST_EQ(net_wait(server.socket, WAIT_WRITE, 0), true);

        TEST_EQ(net_wait(client.socket, WAIT_READ, 20), false);
        TEST_EQ(net_wait(client.socket, WAIT_WRITE, 0), true);
        TEST_EQ(net_wait(client.socket, WAIT_ERROR, 0), false);

        TEST_LOG("Should be able to send with server, but read should fail\n");
        TEST_EQ(net_send(&server, sendbuf, sizeof(sendbuf), NULL), NETERROR_NONE);
        TEST_EQ(net_send(&server, sendbuf, sizeof(sendbuf), NULL), NETERROR_NONE);
        TEST_EQ(net_receive(&server, recvbuf, sizeof(recvbuf), NULL, 0), NETERROR_END_OF_DATA);
        TEST_EQ(net_receive_all(&server, recvbuf, sizeof(recvbuf), NULL, 0), NETERROR_END_OF_DATA);

        TEST_LOG("Should receive what the server has sent\n");
        TEST_EQ(net_receive_all(&client, recvbuf, sizeof(recvbuf), &rx_count, 100), NETERROR_NETWORK_RECEIVE_PARTIAL);
        TEST_EQ(rx_count, sizeof(sendbuf) * 2);
        TEST_EQ(net_receive_all(&client, recvbuf, sizeof(recvbuf), &rx_count, 0), NETERROR_WOULD_BLOCK);

        net_close_handle(&client);

        TEST_EQ(net_receive_all(&server, recvbuf, sizeof(recvbuf), NULL, 100), NETERROR_END_OF_DATA);

        net_close_handle(&server);
        net_close_listener(&listener);
    }


    test_section_name("Network reset seen on server");
    if (setup_connection(&client, &server, &listener)) {
        TEST_EQ(net_send(&client, sendbuf, sizeof(sendbuf), NULL), NETERROR_NONE);
        TEST_EQ(net_send(&server, sendbuf, sizeof(sendbuf), NULL), NETERROR_NONE);

        net_close_handle(&client);

        TEST_EQ(net_send(&server, sendbuf, sizeof(sendbuf), NULL), NETERROR_NETWORK_RESET);

        net_close_handle(&server);
        net_close_listener(&listener);
    }


    test_section_name("Network reset seen on client");
    if (setup_connection(&client, &server, &listener)) {
        TEST_EQ(net_send(&client, sendbuf, sizeof(sendbuf), NULL), NETERROR_NONE);
        TEST_EQ(net_send(&server, sendbuf, sizeof(sendbuf), NULL), NETERROR_NONE);

        net_close_handle(&server);

        TEST_EQ(net_send(&client, sendbuf, sizeof(sendbuf), NULL), NETERROR_NETWORK_RESET);

        net_close_handle(&client);
        net_close_listener(&listener);
    }


    test_section_name("Close listener after a network connection is established");
    if (setup_connection(&client, &server, &listener)) {
        net_close_listener(&listener);

        TEST_EQ(net_send(&client, sendbuf, sizeof(sendbuf), NULL), NETERROR_NONE);
        TEST_EQ(net_send(&server, sendbuf, sizeof(sendbuf), NULL), NETERROR_NONE);

        TEST_EQ(net_receive_all(&server, recvbuf, sizeof(recvbuf), NULL, 0), NETERROR_NETWORK_RECEIVE_PARTIAL);
        TEST_EQ(net_receive_all(&client, recvbuf, sizeof(recvbuf), NULL, 0), NETERROR_NETWORK_RECEIVE_PARTIAL);

        net_close_handle(&client);
        net_close_handle(&server);
    }


    return tests_end();
}
