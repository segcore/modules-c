/*
 * testing.h - testing library for C - public domain
 *
 * https://github.com/segcore/modules-c
 *
 * Licence: see end of file.

 Sample usage:
    #include "testing.h"

    int main(int argc, char** argv) {
        tests_arg_parse(&argc, &argv);

        TEST_EQ(1, 1);
        TEST_NE(1, 2);

        return tests_end();
    }
 */
#ifndef TESTING_H
#define TESTING_H

// Whether to include the function implementations. Only one C file in an
// executable should have this enabled.
//
// In most cases, this is used in stand-alone test files, so defaults to ON.
// To disable it, turn it off with #define TESTING_IMPLEMENTATION 0
#if !defined TESTING_IMPLEMENTATION
    #define  TESTING_IMPLEMENTATION 1
#endif

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

void tests_arg_parse(int* argc, char*** argv); // Default parsing of command line arguments. Optional.
void tests_arg_parse_const(int* argc, const char*** argv);
int tests_end(void);                           // End the test. Usually return this result from main().
void tests_reset(void);                        // Reset pass/fail counts
void test_section_name(const char* name);      // Labels the test output

#define TEST(expr)      _test_assertion(__FILE__, __FUNCTION__, __LINE__, (expr), "%s", #expr)
#define TEST_PASS(...)  _test_assertion(__FILE__, __FUNCTION__, __LINE__, true , __VA_ARGS__)
#define TEST_FAIL(...)  _test_assertion(__FILE__, __FUNCTION__, __LINE__, false, __VA_ARGS__)
#define REQUIRE(expr)   (_test_assertion(__FILE__, __FUNCTION__, __LINE__, (expr), "%s", #expr) || (exit(tests_end()), 0))

// Numeric comparisons
#define TEST_CLOSE(a, b, eps)  _test_close(__FILE__, __FUNCTION__, __LINE__, (a), (b), (eps), #a " ~= " #b)
#define TEST_EQ(a, b)  _test_eq(__FILE__, __FUNCTION__, __LINE__, (a), (b), #a " == " #b)
#define TEST_NE(a, b)  _test_ne(__FILE__, __FUNCTION__, __LINE__, (a), (b), #a " != " #b)
#define TEST_LT(a, b)  _test_lt(__FILE__, __FUNCTION__, __LINE__, (a), (b), #a " < " #b)
#define TEST_LE(a, b)  _test_le(__FILE__, __FUNCTION__, __LINE__, (a), (b), #a " <= " #b)
#define TEST_GT(a, b)  _test_gt(__FILE__, __FUNCTION__, __LINE__, (a), (b), #a " > " #b)
#define TEST_GE(a, b)  _test_ge(__FILE__, __FUNCTION__, __LINE__, (a), (b), #a " >= " #b)

// Integer comparisons
#define TEST_EQI(a, b)  _test_eqi(__FILE__, __FUNCTION__, __LINE__, (a), (b), #a " == " #b)
#define TEST_NEI(a, b)  _test_nei(__FILE__, __FUNCTION__, __LINE__, (a), (b), #a " != " #b)
#define TEST_LTI(a, b)  _test_lti(__FILE__, __FUNCTION__, __LINE__, (a), (b), #a " < " #b)
#define TEST_LEI(a, b)  _test_lei(__FILE__, __FUNCTION__, __LINE__, (a), (b), #a " <= " #b)
#define TEST_GTI(a, b)  _test_gti(__FILE__, __FUNCTION__, __LINE__, (a), (b), #a " > " #b)
#define TEST_GEI(a, b)  _test_gei(__FILE__, __FUNCTION__, __LINE__, (a), (b), #a " >= " #b)

// Other comparisons
#define TEST_ARRAY_EQ(a1, a2, n1, n2)   _test_array_eq(__FILE__, __FUNCTION__, __LINE__, (a1), (a2), (sizeof((a1)[0])), (n1), (n2), #a1 " === " #a2)

#define MAXOF(a, b)     (((a) > (b)) ? (a) : (b))
#define MINOF(a, b)     (((a) < (b)) ? (a) : (b))
#define ABSOF(a)        (((a) < 0) ? -(a) : (a))
#define ARRAY_LENGTH(x) (sizeof ((x)) / sizeof ((x)[0]))


// Public globals
extern size_t tests_passed;
extern size_t tests_failed;
extern bool tests_print_success;
extern const char* tests_current;

// Internal function declarations
bool _test_assertion(const char* file, const char* function, int line, bool expr, const char* format, ...);
bool _test_array_eq(const char* file, const char* function, int line, const void* a1, const void* a2, size_t elem_size, size_t len1, size_t len2, const char* expression);

bool _test_eq(const char* file, const char* function, int line, long double a, long double b, const char* raw_expression);
bool _test_lt(const char* file, const char* function, int line, long double a, long double b, const char* raw_expression);
bool _test_le(const char* file, const char* function, int line, long double a, long double b, const char* raw_expression);
bool _test_gt(const char* file, const char* function, int line, long double a, long double b, const char* raw_expression);
bool _test_ge(const char* file, const char* function, int line, long double a, long double b, const char* raw_expression);

bool _test_eqi(const char* file, const char* function, int line, long long a, long long b, const char* raw_expression);
bool _test_lti(const char* file, const char* function, int line, long long a, long long b, const char* raw_expression);
bool _test_lei(const char* file, const char* function, int line, long long a, long long b, const char* raw_expression);
bool _test_gti(const char* file, const char* function, int line, long long a, long long b, const char* raw_expression);
bool _test_gei(const char* file, const char* function, int line, long long a, long long b, const char* raw_expression);


/// ----- Implementation -----
#if TESTING_IMPLEMENTATION

    // In case this c file includes other files which include testing.h
    #undef TESTING_IMPLEMENTATION
    #define TESTING_IMPLEMENTATION 0

#include <stdarg.h>
size_t tests_passed = 0;
size_t tests_failed = 0;
bool tests_print_success = true;
const char* tests_current = NULL;
void test_section_name(const char* name) {
    tests_current = name;
}

bool _test_close(const char* file, const char* function, int line, long double a, long double b, long double epsilon, const char* raw_expression) {
    bool expr = ABSOF(a - b) <= epsilon;
    return _test_assertion(file, function, line, expr, "%s  [%Lf %s %Lf]", raw_expression, a, expr ? "~=" : "!~=", b);
}
bool _test_eq(const char* file, const char* function, int line, long double a, long double b, const char* raw_expression) {
    bool expr = a == b;
    return _test_assertion(file, function, line, expr, "%s  [%Lf %s %Lf]", raw_expression, a, expr ? "==" : "!=", b);
}
bool _test_ne(const char* file, const char* function, int line, long double a, long double b, const char* raw_expression) {
    bool expr = a != b;
    return _test_assertion(file, function, line, expr, "%s  [%Lf %s %Lf]", raw_expression, a, expr ? "!=" : "==", b);
}
bool _test_lt(const char* file, const char* function, int line, long double a, long double b, const char* raw_expression) {
    bool expr = a < b;
    return _test_assertion(file, function, line, expr, "%s  [%Lf %s %Lf]", raw_expression, a, expr ? "<" : "!<", b);
}
bool _test_le(const char* file, const char* function, int line, long double a, long double b, const char* raw_expression) {
    bool expr = a <= b;
    return _test_assertion(file, function, line, expr, "%s  [%Lf %s %Lf]", raw_expression, a, expr ? "<=" : "!<=", b);
}
bool _test_gt(const char* file, const char* function, int line, long double a, long double b, const char* raw_expression) {
    bool expr = a > b;
    return _test_assertion(file, function, line, expr, "%s  [%Lf %s %Lf]", raw_expression, a, expr ? ">" : "!>", b);
}
bool _test_ge(const char* file, const char* function, int line, long double a, long double b, const char* raw_expression) {
    bool expr = a >= b;
    return _test_assertion(file, function, line, expr, "%s  [%Lf %s %Lf]", raw_expression, a, expr ? ">=" : "!>=", b);
}

bool _test_eqi(const char* file, const char* function, int line, long long a, long long b, const char* raw_expression) {
    bool expr = a == b;
    return _test_assertion(file, function, line, expr, "%s  [%lld %s %lld]", raw_expression, a, expr ? "==" : "!=", b);
}
bool _test_nei(const char* file, const char* function, int line, long long a, long long b, const char* raw_expression) {
    bool expr = a != b;
    return _test_assertion(file, function, line, expr, "%s  [%lld %s %lld]", raw_expression, a, expr ? "!=" : "==", b);
}
bool _test_lti(const char* file, const char* function, int line, long long a, long long b, const char* raw_expression) {
    bool expr = a < b;
    return _test_assertion(file, function, line, expr, "%s  [%lld %s %lld]", raw_expression, a, expr ? "<" : "!<", b);
}
bool _test_lei(const char* file, const char* function, int line, long long a, long long b, const char* raw_expression) {
    bool expr = a <= b;
    return _test_assertion(file, function, line, expr, "%s  [%lld %s %lld]", raw_expression, a, expr ? "<=" : "!<=", b);
}
bool _test_gti(const char* file, const char* function, int line, long long a, long long b, const char* raw_expression) {
    bool expr = a > b;
    return _test_assertion(file, function, line, expr, "%s  [%lld %s %lld]", raw_expression, a, expr ? ">" : "!>", b);
}
bool _test_gei(const char* file, const char* function, int line, long long a, long long b, const char* raw_expression) {
    bool expr = a >= b;
    return _test_assertion(file, function, line, expr, "%s  [%lld %s %lld]", raw_expression, a, expr ? ">=" : "!>=", b);
}

bool _test_assertion(const char* file, const char* function, int line, bool expr, const char* format, ...) {
    // Simplify file paths
    // const char* f1 = strrchr(file, '/');
    // const char* f2 = strrchr(file, '\\');
    // file = f1 ? (f2 ? (f2 > f1 ? f2 : f1) : f1) + 1
    //         : (f2 ? f2 + 1 : file);

    if(expr) {
        tests_passed++;
        if(tests_print_success) {
            fprintf(stderr, "+pass  %s:%d %s(): ", file, line, function);
            if (tests_current) fprintf(stderr, "in '%s': ", tests_current);
            va_list args;
            va_start(args, format);
            vfprintf(stderr, format, args);
            va_end(args);
            fprintf(stderr, "\n");
        }
    } else {
        tests_failed++;
        fprintf(stderr, "-FAIL  %s:%d %s(): ", file, line, function);
        if (tests_current) fprintf(stderr, "in '%s': ", tests_current);
        va_list args;
        va_start(args, format);
        vfprintf(stderr, format, args);
        va_end(args);
        fprintf(stderr, "\n");
    }
    return expr;
}
bool _test_array_eq(const char* file, const char* function, int line, const void* a1, const void* a2, size_t elem_size, size_t len1, size_t len2, const char* expression) {
    bool match = len1 == len2 && memcmp(a1, a2, elem_size * len1) == 0;
    if(!_test_assertion(file, function, line, match, "%s", expression)) {
        fprintf(stderr, "  a1 [%zu]: ", len1);
        for(size_t i = 0; i < MINOF(len1, 99u); i++) fprintf(stderr, " %02x", ((const unsigned char*)a1)[i]);
        fprintf(stderr, "\n  a2 [%zu]: ", len2);
        for(size_t i = 0; i < MINOF(len2, 99u); i++) fprintf(stderr, " %02x", ((const unsigned char*)a2)[i]);
        fprintf(stderr, "\n");
    }
    return match;
}
void tests_reset(void) {
    tests_passed = 0;
    tests_failed = 0;
}
int tests_end(void) {
    printf("=================================\n");
    size_t total = tests_passed + tests_failed;
    size_t percent = (total == 0) ? 0 : (100 * tests_passed / total);
    printf("Passed  %3zu / %3zu   %3zu%%\n", tests_passed, total, percent);
    return tests_failed > 0;
}
void tests_arg_parse(int* argc, char*** argv) {
    while(*argc > 1) {
        if(strcmp((*argv)[1], "-v") == 0) {
            tests_print_success = true;
        } else if(strcmp((*argv)[1], "-q") == 0) {
            tests_print_success = false;
        } else {
            break;
        }
        (*argc)--;
        (*argv)++;
    }
}
void tests_arg_parse_const(int* argc, const char*** argv) { tests_arg_parse(argc, (char***) argv); }
#endif // TESTING_IMPLEMENTATION

#endif // TESTING_H
/*
------------------------------------------------------------------------------
This software is available under 2 licenses -- choose whichever you prefer.
------------------------------------------------------------------------------
ALTERNATIVE A - MIT License

Copyright (c) 2026 Nicholas Rixson

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
------------------------------------------------------------------------------
ALTERNATIVE B - Public Domain (www.unlicense.org)
This is free and unencumbered software released into the public domain.
Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
software, either in source code form or as a compiled binary, for any purpose,
commercial or non-commercial, and by any means.

In jurisdictions that recognize copyright laws, the author or authors of this
software dedicate any and all copyright interest in the software to the public
domain. We make this dedication for the benefit of the public at large and to
the detriment of our heirs and successors. We intend this dedication to be an
overt act of relinquishment in perpetuity of all present and future rights to
this software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------
*/
