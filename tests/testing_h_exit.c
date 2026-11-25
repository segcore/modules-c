#include "testing.h"
#include <limits.h>

int main(int argc, char** argv) {
    tests_arg_parse(&argc, &argv);

    // This is supposed to terminate the test immediately
    REQUIRE(TEST_EQ(1, 2));

    // If this is reached, then WILL_FAIL of CMake will make this test actually fail.
    return 0;
}
