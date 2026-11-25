# Small re-usable modules for C

Generally single-file public domain (or MIT licenced) libraries for C.
As inspired by the [stb single file libraries](https://github.com/nothings/stb).

Find any useful module, and copy it into your project to use it.

## Build and test all the things here

Build and test directly:

```sh
gcc ./tests/testing_h_positive.c -I. -o testing_h_positive
./testing_h_positive

gcc examples/network_server.c network.c -I. -o network_server
./network_server

gcc examples/network_client.c network.c -I. -o network_client
./network_client
```

Build and test all of the things with CMake:

```sh
cmake . -B build
cmake --build build
ctest --test-dir build
```

Code coverage report (available when using gcc or clang):

```sh
ctest --test-dir build -T Test -T Coverage

# If using gcovr
gcovr --root . --html-details build/gcov-html/ &&
xdg-open build/gcov-html/coverage_details.html

# If using lcov
lcov --directory build --capture --output-file build/lcov-coverage.info &&
genhtml build/lcov-coverage.info --output-directory build/lcov-html/ &&
xdg-open build/lcov-html/index.html
```

## Modules

### testing.h

Library for creating tests. Gives some more helpful output than basic asserts, etc.
See [./testing.h](testing.h) for more supported functions.

Sample code:

```c
#include "testing.h"

int main(int argc, char** argv) {
    tests_arg_parse(&argc, &argv);

    TEST_EQ(1, 1); // Test Equal
    TEST_NE(1, 2); // Test Not-Equal
    TEST_LE(4, 7); // Test less-than-or-equal
    // ...

    double added = 0.1 + 0.2;
    TEST_CLOSE(added, 0.3, 1e-6);

    return tests_end();
}
```

Example output from `./tests/testing_h_positive.c`:

```
$ ./tests/testing_h_positive
...
+pass  /.../tests/testing_h_positive.c:65 main(): medium >= medium  [33 >= 33]
+pass  /.../tests/testing_h_positive.c:68 main(): 1 == 1  [1 == 1]
+pass  /.../tests/testing_h_positive.c:69 main(): 1.5 == 1.5  [1.500000 == 1.500000]
+pass  /.../tests/testing_h_positive.c:70 main(): 12345678 == 12345678  [12345678 == 12345678]
+pass  /.../tests/testing_h_positive.c:71 main(): 1.0 ~= 1.000000004  [1.000000 ~= 1.000000]
+pass  /.../tests/testing_h_positive.c:73 main(): added ~= 0.3  [0.300000 ~= 0.300000]
+pass  /.../tests/testing_h_positive.c:77 main(): length1 != sizeof(length1)  [1234 != 8]
=================================
Passed   50 /  50   100%

$ ./tests/testing_h_negative
...
-FAIL  /.../tests/testing_h_negative.c:39 main(): 6 > 6  [6.000000 !> 6.000000]
-FAIL  /.../tests/testing_h_negative.c:40 main(): 123 >= 150  [123.000000 !>= 150.000000]
...
```

or from ctest (CMake's testing automation). Build an run with:
```sh
cmake . -B build
cmake --build build
ctest --test-dir build
```

CTest output:

```
    Start 1: testing_h_positive
1/3 Test #1: testing_h_positive ...............   Passed    0.00 sec
    Start 2: testing_h_negative
2/3 Test #2: testing_h_negative ...............   Passed    0.00 sec
    Start 3: testing_h_exit
3/3 Test #3: testing_h_exit ...................   Passed    0.00 sec

100% tests passed out of 3

Total Test time (real) =   0.00 sec
```

#### CTest

If using CMake as the build system, adding CTest as a test runner takes very
little effort. Much of the functionality which is built-in to GoogleTest and
Boost.Test is also available in CTest. This means that the testing framework
can be simpler and just focus on testing (which makes it compile faster too).

Some CTest features include:

* Run all tests in one command
* Run a subset of tests
* Run tests in parallel
* Randomize test order
* Test timeouts
* Quiet and verbose modes
* Output JUnit formatted test results
* Repeat tests
* Test cost logging (how long each test takes to run, over many runs)
* Re-run only failed tests
* Submit results to a graphical dashboard

### yar

Convenient, typesafe, single-implementation, zero-copy dynamic arrays in C without crazy macros.

```c
define YAR_IMPLEMENTATION
#include "yar.h"

struct Numbers {
    double *items;
    size_t count;
    size_t capacity;
};

int main() {
    struct Numbers numbers = {0};
    // Or: yar(double) numbers = {0};
    *yar_append(&numbers) = 3.14159;
    *yar_append(&numbers) = 2.71828;
    *yar_append(&numbers) = 1.61803;

    for(size_t i = 0; i < numbers.count; i++) {
        printf("%f\n", numbers.items[i]);
    }

    yar_free(&numbers);
}
```

### network.h

Simple cross-platform networking abstraction. Currently supports TCP clients and servers only.
See examples for [client](./examples/network_client.c) and [server](./examples/network_server.c).

```c
const char* net_strerror(enum NetErrorCode net_error);

void net_init(NetHandle* net);
enum NetErrorCode net_connect(NetHandle* net, const char* remote_name, uint16_t remote_port, int timeout_ms);

void net_init_listener(NetListener* listener);
enum NetErrorCode net_listen(NetListener* listener, const char* local_addr, uint16_t local_port);
enum NetErrorCode net_accept(NetListener* listener, NetHandle* out_accepted, int timeout_ms);

enum NetErrorCode net_send(NetHandle* net, const void* data, size_t size, size_t* bytes_sent);
enum NetErrorCode net_receive(NetHandle* net, void* data, size_t buffer_size, size_t* bytes_received, int timeout_ms);
enum NetErrorCode net_receive_all(NetHandle* net, void* data, size_t buffer_size, size_t* bytes_received, int timeout_ms);

bool net_wait(Socket sock, int wait_type, int timeout_ms);
bool net_shutdown(NetHandle* net, int shutdown_type);
void net_close_socket(Socket sock);
void net_close_handle(NetHandle* net);
void net_close_listener(NetListener* listener);
```
