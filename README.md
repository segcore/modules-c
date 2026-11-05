# Small re-usable modules for C

Generally single-file public domain (or MIT licenced) libraries for C.
As inspired by the [stb single file libraries](https://github.com/nothings/stb).

Find any useful module, and copy it into your project to use it.

## Build and test all the things here

```sh
cmake . -B build
cmake --build build
ctest --test-dir build
```

## Modules

### testing.h

Library for creating tests. Gives some more helpful output than basic asserts, etc.

Sample code:

```c
#include "testing.h"

int main(int argc, char** argv) {
    tests_arg_parse(&argc, &argv);

    TEST_EQ(1, 1);
    TEST_NE(1, 2);

    return tests_end();
}
```

Example output from `./tests/testing_h_positive.c`:

```
$ ./tests/testing_h_positive
...
+pass  /.../tests/testing_h_positive.c:72 main(): medium >= medium  [33 >= 33]
+pass  /.../tests/testing_h_positive.c:75 main(): 1 == 1  [1.000000 == 1.000000]
+pass  /.../tests/testing_h_positive.c:76 main(): 1.5 == 1.5  [1.500000 == 1.500000]
+pass  /.../tests/testing_h_positive.c:77 main(): 12345678 == 12345678  [12345678.000000 == 12345678.000000]
+pass  /.../tests/testing_h_positive.c:78 main(): 1.0 ~= 1.000000004  [1.000000 ~= 1.000000]
+pass  /.../tests/testing_h_positive.c:80 main(): added ~= 0.3  [0.300000 ~= 0.300000]
+pass  /.../tests/testing_h_positive.c:78 main(): length1 != sizeof(length1)  [1234 != 8]
=================================
Passed   50 /  50   100%
```

or from ctest
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
