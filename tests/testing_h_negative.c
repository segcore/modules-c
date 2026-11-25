#include "testing.h"
#include <limits.h>


int main(int argc, char** argv) {
    tests_arg_parse(&argc, &argv);

    // Methodical testing of all macros, in order
    TEST(1 == 2);
    // TEST_PASS("Yes, one is indeed one"); // Tested in the positive tests
    TEST_FAIL("This is indeed wrong");
    // REQUIRE(false); exits the program

    // Integer comparisons
    TEST_EQ(1, 2);
    TEST_EQ(-123456789, -123456788);
    TEST_EQ(LONG_MAX, LONG_MAX-1);
    TEST_EQ(-LONG_MAX, -LONG_MAX+1);
    TEST_EQ(LLONG_MAX, LLONG_MAX-1);
    TEST_EQ(-LLONG_MAX, -LLONG_MAX+1);

    TEST_NE(123, 123);
    TEST_LT(123, 44); TEST_LT(3, 3);
    TEST_LE(55, 32);
    TEST_GT(98, 99); TEST_GT(3, 3);
    TEST_GE(9, 13);

    // Numeric comparisons
    TEST_CLOSE(1, 2, 1e-6);
    TEST_CLOSE(1, 1.001, 1e-6);
    TEST_CLOSE(123, 145, 5);
    TEST_EQF(5, 6); TEST_EQF(-3.14, 3.14);
    TEST_NEF(1, 1);
    TEST_LTF(6, 5); TEST_LTF(4, 4);
    TEST_LEF(9, 5);
    TEST_GTF(55, 123); TEST_GTF(6, 6);
    TEST_GEF(123, 150);

    // Other
    int arr1[] = {1, 2, 3, 4, 5};
    int arr2[] = {1, 2, 3, 5, 5};
    TEST_ARRAY_EQ(arr1, arr2, ARRAY_LENGTH(arr1), ARRAY_LENGTH(arr2));

    char charr1[] = { 'a', 'b', 'c' };
    char charr2[] = { 'x', 'y', 'z' };
    TEST_ARRAY_EQ(charr1, charr2, ARRAY_LENGTH(charr1), ARRAY_LENGTH(charr2));

    int final_pass = tests_passed;
    int final_failed = tests_failed;
    int result = tests_end();
    tests_reset();

    // Final, positive tests asserting about the negative tests above.
    test_section_name("Post-test");
    TEST_NE(result, 0);
    TEST_EQ(final_pass, 0);
    TEST_GT(final_failed, 0);
    return tests_end();
}
