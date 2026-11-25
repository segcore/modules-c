#include "testing.h"
#include <limits.h>


int main(int argc, char** argv) {
    tests_arg_parse(&argc, &argv);

    // Methodical testing of all macros, in order
    TEST(1 == 1);
    TEST_PASS("Yes, one is indeed one");
    // TEST_FAIL(); // tested in the negative test cases
    REQUIRE(true);

    // Integer comparisons
    TEST_EQ(1, 1);
    TEST_EQ(-123456789, -123456789);
    TEST_EQ(LONG_MAX, LONG_MAX);
    TEST_EQ(-LONG_MAX, -LONG_MAX);
    TEST_EQ(LLONG_MAX, LLONG_MAX);
    TEST_EQ(-LLONG_MAX, -LLONG_MAX);

    TEST_NE(123, 42);
    TEST_LT(123, 554);
    TEST_LE(55, 70); TEST_LE(44, 44);
    TEST_GT(52, 6);
    TEST_GE(5, -1); TEST_GE(33, 33);

    TEST_EQ(MAXOF(42, 55), 55);
    TEST_EQ(MINOF(42, 55), 42);
    TEST_EQ(ABSOF(13), 13);
    TEST_EQ(ABSOF(-13), 13);
    int some_array[5];
    TEST_EQ(ARRAY_LENGTH(some_array), 5);

    // Numeric comparisons
    TEST_CLOSE(1, 1, 1e-6);
    TEST_CLOSE(1, 1.000000001, 1e-6);
    TEST_CLOSE(123, 145, 30);
    TEST_EQF(5, 5); TEST_EQF(-3.14, -3.14);
    TEST_NEF(1, 2);
    TEST_LTF(1, 5);
    TEST_LEF(4, 5); TEST_LEF(5, 5);
    TEST_GTF(123, 55);
    TEST_GEF(123, 15); TEST_GEF(123, 123);

    // Other
    int arr1[] = {1, 2, 3, 4, 5};
    int arr2[] = {1, 2, 3, 4, 5};
    TEST_ARRAY_EQ(arr1, arr2, ARRAY_LENGTH(arr1), ARRAY_LENGTH(arr2));

    char charr1[] = { 'a', 'b', 'c' };
    char charr2[] = { 'a', 'b', 'c' };
    TEST_ARRAY_EQ(charr1, charr2, ARRAY_LENGTH(charr1), ARRAY_LENGTH(charr2));

    // Unstructed testing
    REQUIRE(3 > 0);
    REQUIRE(TEST_EQ(1234, 1234));

    int x = 1234;
    int y = 4321;
    TEST(y > x);

    int small = 12;
    int medium = 33;
    int large = 44;
    TEST_LT(small, medium);
    TEST_LE(small, large);
    TEST_LE(medium, medium);
    TEST_GT(large, small);
    TEST_GE(large, small);
    TEST_GE(medium, medium);


    TEST_EQ(1, 1);
    TEST_EQF(1.5, 1.5);
    TEST_EQ(12345678, 12345678);
    TEST_CLOSE(1.0, 1.000000004, 1e-6);
    double added = 0.1 + 0.2;
    TEST_CLOSE(added, 0.3, 1e-6);

    // Do unsigned integers complain? Not in my testing
    size_t length1 = 1234;
    TEST_NE(length1, sizeof(length1));

    return tests_end();
}
