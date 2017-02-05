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
    set_default_context(&ctx);

    TSCode res;

    res = register_pid(NULL, 0x100);
    test_assert_equal(res, TSD_INVALID_CONTEXT, "invlaid context");
    res = register_pid(&ctx, 0x100);
    test_assert_equal(res, TSD_OK, "valid PID registration");
    res = register_pid(&ctx, 0x100);
    test_assert_equal(res, TSD_PID_ALREADY_REGISTERED, "PID already registered");

    int i;
    for(i=1; i< MAX_PID_REGISTRATIONS; ++i) {
        res = register_pid(&ctx, 0x400 + i);
        test_assert_equal(res, TSD_OK, "valid PID registration");
    }

    res = register_pid(&ctx, 0xFFFF);
    test_assert_equal(res, TSD_MAX_PID_REGISTRATIONS_REACHED, "max PID registration");

    test_end();
}

void test_deregister(void)
{
    test_start("deregister pid");

    TSDemuxContext ctx;
    set_default_context(&ctx);

    TSCode res;

    res = register_pid(NULL, 0x100);
    test_assert_equal(res, TSD_INVALID_CONTEXT, "invlaid context");

    test_end();
}
