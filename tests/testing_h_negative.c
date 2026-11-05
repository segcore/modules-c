#include "testing.h"
#include <limits.h>


int main(int argc, char** argv) {
    tests_arg_parse(&argc, &argv);

    // Methodical testing of all macros, in order
    TEST(1 == 2);
    // TEST_PASS("Yes, one is indeed one"); // Tested in the positive tests
    TEST_FAIL("This is indeed wrong");
    // int arr1[] = {1, 2, 3, 4, 5};
    // int arr2[] = {1, 2, 3, 5, 5};
    // TEST_ARRAY_EQ(arr1, arr2, ARRAY_LENGTH(arr1), ARRAY_LENGTH(arr2));
    // REQUIRE(false); exits the program

    // Numeric comparisons
    TEST_CLOSE(1, 2, 1e-6);
    TEST_CLOSE(1, 1.001, 1e-6);
    TEST_CLOSE(123, 145, 5);
    TEST_EQ(5, 6); TEST_EQ(-3.14, 3.14);
    TEST_NE(1, 1);
    TEST_LT(6, 5); TEST_LT(4, 4);
    TEST_LE(9, 5);
    TEST_GT(55, 123); TEST_GT(6, 6);
    TEST_GE(123, 150);

    // Integer comparisons
    TEST_EQI(1, 2);
    TEST_EQI(-123456789, -123456788);
    TEST_EQI(LONG_MAX, LONG_MAX-1);
    TEST_EQI(-LONG_MAX, -LONG_MAX+1);
    TEST_EQI(LLONG_MAX, LLONG_MAX-1);
    TEST_EQI(-LLONG_MAX, -LLONG_MAX+1);

    TEST_NEI(123, 123);
    TEST_LTI(123, 44); TEST_LTI(3, 3);
    TEST_LEI(55, 32);
    TEST_GTI(98, 99); TEST_GTI(3, 3);
    TEST_GEI(9, 13);

    int final_pass = tests_passed;
    int final_failed = tests_failed;
    int result = tests_end();
    tests_reset();

    // Final, positive tests asserting about the negative tests above.
    test_section_name("Post-test");
    TEST_NEI(result, 0);
    TEST_EQI(final_pass, 0);
    TEST_GTI(final_failed, 0);
    return tests_end();
}
