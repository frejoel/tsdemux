#include "test.h"
#include <tsdemux.h>
#include <string.h>
#include <stdio.h>

void test_register(void);
void test_deregister(void);

int main(char **argc, int argv)
{
    test_register();
    test_deregister();
    return 0;
}

void test_register(void)
{
    test_start("register pid");

    TSDemuxContext ctx;
    tsd_context_init(&ctx);

    TSDCode res;

    res = tsd_register_pid(NULL, 0x100);
    test_assert_equal(res, TSD_INVALID_CONTEXT, "invlaid context");
    res = tsd_register_pid(&ctx, 0x100);
    test_assert_equal(res, TSD_OK, "valid PID registration");
    res = tsd_register_pid(&ctx, 0x100);
    test_assert_equal(res, TSD_PID_ALREADY_REGISTERED, "PID already registered");

    int i;
    for(i=1; i< TSD_MAX_PID_REGS; ++i) {
        res = tsd_register_pid(&ctx, 0x400 + i);
        test_assert_equal(res, TSD_OK, "valid PID registration");
    }

    res = tsd_register_pid(&ctx, 0xFFFF);
    test_assert_equal(res, TSD_TSD_MAX_PID_REGS_REACHED, "max PID registration");

    test_end();
}

void test_deregister(void)
{
    test_start("deregister pid");

    TSDemuxContext ctx;
    tsd_context_init(&ctx);

    TSDCode res;

    res = tsd_deregister_pid(NULL, 0x100);
    test_assert_equal(res, TSD_INVALID_CONTEXT, "invalid context");
    res = tsd_deregister_pid(&ctx, 0x100);
    test_assert_equal(res, TSD_PID_NOT_FOUND, "pid not found");
    res = tsd_register_pid(&ctx, 0x200);
    res = tsd_deregister_pid(&ctx, 0x200);
    test_assert_equal(res, TSD_OK, "remove pid");
    res = tsd_register_pid(&ctx, 0x200);
    test_assert_equal(res, TSD_OK, "register same PID");
    res = tsd_deregister_pid(&ctx, 0x200);
    test_assert_equal(res, TSD_OK, "remove same PID again");

    test_end();
}
