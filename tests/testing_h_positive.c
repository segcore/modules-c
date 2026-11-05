#include "testing.h"
#include <limits.h>
#include <stdlib.h>


int main(int argc, char** argv) {
    tests_arg_parse(&argc, &argv);

    // Methodical testing of all macros, in order
    TEST(1 == 1);
    TEST_PASS("Yes, one is indeed one");
    // TEST_FAIL(); // tested in the negative test cases
    int arr1[] = {1, 2, 3, 4, 5};
    int arr2[] = {1, 2, 3, 4, 5};
    TEST_ARRAY_EQ(arr1, arr2, ARRAY_LENGTH(arr1), ARRAY_LENGTH(arr2));
    REQUIRE(true);

    // Numeric comparisons
    TEST_CLOSE(1, 1, 1e-6);
    TEST_CLOSE(1, 1.000000001, 1e-6);
    TEST_CLOSE(123, 145, 30);
    TEST_EQ(5, 5); TEST_EQ(-3.14, -3.14);
    TEST_NE(1, 2);
    TEST_LT(1, 5);
    TEST_LE(4, 5); TEST_LE(5, 5);
    TEST_GT(123, 55);
    TEST_GE(123, 15); TEST_GE(123, 123);

    // Integer comparisons
    TEST_EQI(1, 1);
    TEST_EQI(-123456789, -123456789);
    TEST_EQI(LONG_MAX, LONG_MAX);
    TEST_EQI(-LONG_MAX, -LONG_MAX);
    TEST_EQI(LLONG_MAX, LLONG_MAX);
    TEST_EQI(-LLONG_MAX, -LLONG_MAX);

    TEST_NEI(123, 42);
    TEST_LTI(123, 554);
    TEST_LEI(55, 70); TEST_LEI(44, 44);
    TEST_GTI(52, 6);
    TEST_GEI(5, -1); TEST_GEI(33, 33);

    TEST_EQI(MAXOF(42, 55), 55);
    TEST_EQI(MINOF(42, 55), 42);
    TEST_EQI(ABSOF(13), 13);
    TEST_EQI(ABSOF(-13), 13);
    TEST_EQI(ARRAY_LENGTH(arr1), 5);


    // Unstructed testing
    REQUIRE(3 > 0);
    REQUIRE(TEST_EQI(1234, 1234));

    int x = 1234;
    int y = 4321;
    TEST(y > x);

    int small = 12;
    int medium = 33;
    int large = 44;
    TEST_LTI(small, medium);
    TEST_LEI(small, large);
    TEST_LEI(medium, medium);
    TEST_GTI(large, small);
    TEST_GEI(large, small);
    TEST_GEI(medium, medium);


    TEST_EQ(1, 1);
    TEST_EQ(1.5, 1.5);
    TEST_EQ(12345678, 12345678);
    TEST_CLOSE(1.0, 1.000000004, 1e-6);
    double added = 0.1 + 0.2;
    TEST_CLOSE(added, 0.3, 1e-6);

    // Do unsigned integers complain? Not in my testing
    size_t length1 = 1234;
    TEST_NEI(length1, sizeof(length1));

    return tests_end();
}
