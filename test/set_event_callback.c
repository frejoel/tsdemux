#include "test.h"
#include <tsdemux.h>

void test_set_callback(void);

int main(int argc, char **argv)
{
    test_set_callback();
    return 0;
}

void callback(TSDemuxContext *ctx, uint16_t pid, TSDEventId id, void *data)
{

}

void test_set_callback(void)
{
    test_start("tsd_set_event_callback");

    TSDemuxContext ctx;
    tsd_context_init(&ctx);

    TSDCode res = tsd_set_event_callback(NULL, NULL);
    test_assert_equal(TSD_INVALID_CONTEXT, res, "invalid context");

    res = tsd_set_event_callback(&ctx, callback);
    test_assert_equal(TSD_OK, res, "adding context");
    test_assert_equal_ptr((size_t)ctx.event_cb, (size_t)callback, "callback is set");

    res = tsd_set_event_callback(&ctx, NULL);
    test_assert_equal(TSD_OK, res, "NULL callback");
    test_assert_equal_ptr((size_t)ctx.event_cb, (size_t)NULL, "callback is set to NULL");

    test_end();
}
