#ifndef TEST_H
#define TEST_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <tsdemux.h>
#include <malloc.h>

#ifndef LOG_ALLOCATION
#  define LOG_ALLOCATION 0
#endif

#ifndef ALLOC_MEM_INIT 
#  define ALLOC_MEM_INIT 0xF1
#endif

// store allocated size in first bytes
static void* test_malloc (size_t size) {
    size_t alloc_size = size+sizeof(size_t);
    size_t *result = malloc(alloc_size);
    // set to value other then 0
    memset(result, ALLOC_MEM_INIT, alloc_size);
    *result = size;
#if LOG_ALLOCATION
    printf("malloc(%d) -> %p %s\n", (int)size,result, result!=NULL?"OK":"ERROR");
#endif
    return &result[1];
}

static void* test_calloc(size_t num, size_t size){
    size_t alloc_size = size+sizeof(size_t);
    size_t *result = calloc(num, alloc_size);
    // store size in header
    *result = size;
#if LOG_ALLOCATION
    printf("calloc(%d) -> %p %s\n", (int)(num*size),result, result!=NULL?"OK":"ERROR");
#endif
    return &result[1];
}

static void* test_realloc(void *ptr, size_t size){
    size_t alloc_size = size+sizeof(size_t);
    size_t* size_p = (size_t*) ptr;
    size_t old_size = size_p[-1]; 
    size_t *result = realloc(&size_p[-1], alloc_size);
    // store size in header
    *result = size;
    uint8_t *data = (uint8_t*) &result[1];
    // set to value other then 0
    memset(data+old_size, ALLOC_MEM_INIT, size-old_size);
#if LOG_ALLOCATION
    printf("realloc(%d) -> %p %s\n", (int)size, result, result!=NULL?"OK":"ERROR");
#endif
    return data;
}

static void test_free (void *mem){
#if LOG_ALLOCATION
    printf("free(%p)\n", mem);
#endif
    size_t *start = mem;
    free(&start[-1]);
}


static TSDCode test_context_init(TSDemuxContext *ctx) {
    TSDCode result = tsd_context_init(ctx);
    ctx->malloc = test_malloc;
    ctx->calloc = test_calloc;
    ctx->realloc = test_realloc;
    ctx->free = test_free;

    return result;
}

static void test_start(const char *name)
{
    printf("\n------ test start ------\n  RUNNING: %s\n", name);
}

static void test_end()
{
    printf("  PASS\n------ test end ------\n\n");
}

static void test_assert(int value, const char *msg)
{
    if(value == 0) {
        printf("      ASSERT: %s\n  FAIL\n------ test end ------\n\n", msg);
#ifndef ARDUINO
        abort();
#else
        // endless loop;
        while(1);
#endif
    } else {
        printf("      OK: %s\n", msg);
    }
}

static void test_assert_equal(int val1, int val2, const char *msg)
{
    test_assert(val1 == val2, msg);
}

static void test_assert_equal_uint64(uint64_t val1, uint64_t val2, const char *msg)
{
    test_assert(val1 == val2, msg);
}

static void test_assert_equal_ptr(size_t val1, size_t val2, const char *msg)
{
    test_assert(val1 == val2, msg);
}

static void test_assert_null(const void *ptr, const char *msg)
{
    test_assert(ptr != NULL, msg);
}

static void test_assert_not_equal(int val1, int val2, const char *msg)
{
    test_assert(val1 != val2, msg);
}

#endif // TEST_H
