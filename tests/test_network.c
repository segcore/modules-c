#include "testing.h"
#include "network.h"

int main(int argc, char* argv[])
{
    tests_arg_parse(&argc, &argv);
    NetHandle client;
    NetHandle server;
    NetListener listener;

    net_init(&client);
    net_init(&server);
    net_init_listener(&listener);

    enum NetErrorCode ec = NETERROR_NONE;
    test_section_name("Establish connection");
    TEST_MSG((ec = net_listen(&listener, "127.0.0.1", 42069)) == NETERROR_NONE, "Listening");
    ec = net_connect(&client, "127.0.0.1", 42069, 0);
    TEST_MSG(ec == NETERROR_NONE || ec == NETERROR_CONNECTING, "Should connect");
    TEST_MSG(net_wait(client.socket, WAIT_WRITE, 50), "Waiting for connect to succeed");
    TEST_MSG((ec = net_connect(&client, "127.0.0.1", 42069, 0)) == NETERROR_NONE, "Ensure connect successful");

    TEST_MSG((ec = net_accept(&listener, &server, 0)) == NETERROR_NONE, "Server should successfully accept the connection");

    test_section_name("Write from client");
    {
        char write_buf[] = "Hello from client";
        char read_buf[256] = "";
        size_t bytes_read = 0;
        TEST(net_send(&client, write_buf, sizeof(write_buf), NULL) == NETERROR_NONE);
        TEST(net_receive(&server, read_buf, sizeof(read_buf), &bytes_read, 10) == NETERROR_NONE);
        TEST_ARRAY_EQ(read_buf, write_buf, bytes_read, sizeof(write_buf));
    }

    test_section_name("Write from server");
    {
        char write_buf[] = "Hello from server";
        char read_buf[256] = "";
        size_t bytes_read = 0;
        TEST(net_send(&server, write_buf, sizeof(write_buf), NULL) == NETERROR_NONE);
        TEST(net_receive(&client, read_buf, sizeof(read_buf), &bytes_read, 10) == NETERROR_NONE);
        TEST_ARRAY_EQ(read_buf, write_buf, bytes_read, sizeof(write_buf));
    }

    test_section_name("Close");
    net_close_handle(&client);
    TEST_PASS("Closed client socket");

    unsigned char x[1];
    TEST_MSG(net_wait(server.socket, WAIT_ANY, 20), "Server wait should detect closure quickly");
    printf("Subsequent read should say END_OF_DATA\n");
    TEST_EQ(ec = net_receive(&server, x, sizeof(x), NULL, 0), NETERROR_END_OF_DATA);

    net_close_handle(&server);
    net_close_listener(&listener);
    TEST_PASS("Closed server socket");

    return tests_end();
}
