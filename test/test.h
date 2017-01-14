#ifndef TEST_H
#define TEST_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

void test_start(const char *name)
{
    printf("\n------ test start ------\n  RUNNING: %s\n", name);
}

void test_end()
{
    printf("  PASS\n------ test end ------\n\n");
}

void test_assert(int value, const char *msg)
{
    if(value == 0) {
        printf("      ASSERT: %s\n  FAIL\n------ test end ------\n\n", msg);
        abort();
    } else {
        printf("      OK: %s\n", msg);
    }
}

void test_assert_equal(int val1, int val2, const char *msg)
{
    test_assert(val1 == val2, msg);
}

void test_assert_equal_uint64(uint64_t val1, uint64_t val2, const char *msg)
{
    test_assert(val1 == val2, msg);
}

void test_assert_equal_ptr(size_t val1, size_t val2, const char *msg)
{
    test_assert(val1 == val2, msg);
}

void test_assert_null(const void *ptr, const char *msg)
{
    test_assert(ptr != NULL, msg);
}

void test_assert_not_equal(int val1, int val2, const char *msg)
{
    test_assert(val1 != val2, msg);
}

#endif // TEST_H
