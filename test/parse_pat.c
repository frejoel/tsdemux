#include "test.h"
#include <tsdemux.h>

void test_parse_pat_input(void);

int main(int argc, char **argv)
{
    test_parse_pat_input();
    return 0;
}

void test_parse_pat_input(void)
{
    test_start("parse_pat input");

    TSDemuxContext ctx;
    PATData pat;
    TSCode res;

    set_default_context(&ctx);
    memset(&pat, 0, sizeof(pat));

    uint8_t data[] = {
        0x00, 0x00, 0x00, 0x00,
    };

    res = parse_pat(NULL, NULL, 0, NULL);
    test_assert_equal(TSD_INVALID_CONTEXT, res, "invalid context");
    res = parse_pat(&ctx, NULL, 0, NULL);
    test_assert_equal(TSD_INVALID_DATA, res, "invalid data");
    res = parse_pat(&ctx, data, 3, NULL);
    test_assert_equal(TSD_INVALID_DATA_SIZE, res, "invalid size");
    res = parse_pat(&ctx, data, sizeof(data), NULL);
    test_assert_equal(TSD_INVALID_ARGUMENT, res, "invalid argument");
    res = parse_pat(&ctx, data, sizeof(data), &pat);
    test_assert_equal(TSD_OK, res, "successful parse");

    test_end();
}
