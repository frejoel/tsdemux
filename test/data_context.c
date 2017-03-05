#include "test.h"
#include <tsdemux.h>

void test_init(void);
void test_destroy(void);
void test_write(void);
void test_reset(void);

int main(int argc, char** argv)
{
    test_init();
    test_destroy();
    test_write();
    test_reset();
    return 0;
}

void test_init(void)
{
    test_start("tsd_data_context_init");

    TSDCode res;
    TSDemuxContext ctx;
    TSDDataContext dataCtx;

    tsd_context_init(&ctx);

    res = tsd_data_context_init(NULL, &dataCtx);
    test_assert_equal(res, TSD_INVALID_CONTEXT, "null context");
    res = tsd_data_context_init(&ctx, NULL);
    test_assert_equal(res, TSD_INVALID_ARGUMENT, "null data context");
    res = tsd_data_context_init(NULL, NULL);
    test_assert_equal(res, TSD_INVALID_CONTEXT, "null context and data context");
    res = tsd_data_context_init(&ctx, &dataCtx);
    test_assert_equal(res, TSD_OK, "valid context");

    test_assert(dataCtx.buffer != NULL, "buffer");
    test_assert(dataCtx.size, "size");
    test_assert_equal_ptr((size_t)dataCtx.buffer, (size_t)dataCtx.write, "write position");
    test_assert_equal_ptr((size_t)&dataCtx.buffer[dataCtx.size], (size_t)dataCtx.end, "end position");

    tsd_data_context_destroy(&ctx, &dataCtx);

    test_end();
}

void test_destroy(void)
{
    test_start("tsd_data_context_destroy");

    TSDCode res;
    TSDemuxContext ctx;
    TSDDataContext dataCtx;

    tsd_context_init(&ctx);
    res = tsd_data_context_init(&ctx, &dataCtx);
    test_assert_equal(TSD_OK, res, "init");

    res = tsd_data_context_destroy(NULL, &dataCtx);
    test_assert_equal(res, TSD_INVALID_CONTEXT, "null context");
    res = tsd_data_context_destroy(&ctx, NULL);
    test_assert_equal(res, TSD_INVALID_ARGUMENT, "null data context");
    res = tsd_data_context_destroy(NULL, NULL);
    test_assert_equal(res, TSD_INVALID_CONTEXT, "null context and data context");
    res = tsd_data_context_destroy(&ctx, &dataCtx);
    test_assert_equal(res, TSD_OK, "valid context");

    test_assert(dataCtx.buffer == NULL, "buffer");
    test_assert_equal(0, dataCtx.size, "size");
    test_assert_equal_ptr(0, (size_t)dataCtx.buffer, "null buffer");
    test_assert_equal_ptr(0, (size_t)dataCtx.write, "null write");
    test_assert_equal_ptr(0, (size_t)dataCtx.end, "null end");

    test_end();
}

void test_write(void)
{
    test_start("tsd_data_context_write");

    TSDCode res;
    TSDemuxContext ctx;
    TSDDataContext dataCtx;

    tsd_context_init(&ctx);
    res = tsd_data_context_init(&ctx, &dataCtx);
    test_assert_equal(TSD_OK, res, "init");

    // data we will write into the buffer to test
    uint8_t data[1024];
    int i, j;
    for(i=0; i<1024; ++i) {
        data[i] = (uint8_t)(i / 4);
    }

    res = tsd_data_context_write(NULL, NULL, NULL, 0);
    test_assert_equal(res, TSD_INVALID_CONTEXT, "null context");
    res = tsd_data_context_write(&ctx, NULL, NULL, 0);
    test_assert_equal(res, TSD_INVALID_ARGUMENT, "null data context");
    res = tsd_data_context_write(&ctx, &dataCtx, NULL, 0);
    test_assert_equal(res, TSD_INVALID_DATA, "null context and data context");
    res = tsd_data_context_write(&ctx, &dataCtx, data, 0);
    test_assert_equal(res, TSD_INVALID_DATA_SIZE, "null context and data context");

    res = tsd_data_context_write(&ctx, &dataCtx, data, 128);
    test_assert_equal(res, TSD_OK, "valid context");
    res = tsd_data_context_write(&ctx, &dataCtx, data+128, 128);
    test_assert_equal(res, TSD_OK, "valid context");
    res = tsd_data_context_write(&ctx, &dataCtx, data+256, 250);
    test_assert_equal(res, TSD_OK, "valid context");
    res = tsd_data_context_write(&ctx, &dataCtx, data+506, 294);
    test_assert_equal(res, TSD_OK, "valid context");
    res = tsd_data_context_write(&ctx, &dataCtx, data+800, 224);

    // check that the data is still correct
    for(i=0; i<1024; ++i) {
        if(dataCtx.buffer[i] != (uint8_t)(i / 4)) {
            test_assert(0, "written data is incorrect");
        }
    }

    tsd_data_context_destroy(&ctx,&dataCtx);

    test_end();
}

void test_reset(void)
{
    test_start("tsd_data_context_reset");

    TSDCode res;
    TSDemuxContext ctx;
    TSDDataContext dataCtx;

    tsd_context_init(&ctx);
    res = tsd_data_context_init(&ctx, &dataCtx);
    test_assert_equal(TSD_OK, res, "init");

    // data we will write into the buffer to test
    uint8_t data[1024];
    int i, j;
    for(i=0; i<1024; ++i) {
        data[i] = (uint8_t)(i / 4);
    }

    res = tsd_data_context_reset(&ctx, &dataCtx);
    test_assert_equal(res, TSD_OK, "valid empty reset");
    // the buffer should equal the write position
    test_assert_equal_ptr((size_t)dataCtx.buffer, (size_t)dataCtx.write, "write position");

    res = tsd_data_context_write(&ctx, &dataCtx, data, 512);
    test_assert_equal(res, TSD_OK, "valid context");
    res = tsd_data_context_write(&ctx, &dataCtx, data+512, 512);
    test_assert_equal(res, TSD_OK, "valid context");

    res = tsd_data_context_reset(NULL, NULL);
    test_assert_equal(res, TSD_INVALID_CONTEXT, "invalid context");
    res = tsd_data_context_reset(&ctx, NULL);
    test_assert_equal(res, TSD_INVALID_ARGUMENT, "null data context");
    res = tsd_data_context_reset(NULL, &dataCtx);
    test_assert_equal(res, TSD_INVALID_CONTEXT, "invalid context");
    res = tsd_data_context_reset(&ctx, &dataCtx);
    test_assert_equal(res, TSD_OK, "valid reset");

    // the buffer should equal the write position
    test_assert_equal_ptr((size_t)dataCtx.buffer, (size_t)dataCtx.write, "write position");

    tsd_data_context_destroy(&ctx,&dataCtx);

    test_end();
}
