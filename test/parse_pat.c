#include "test.h"
#include <tsdemux.h>
#include <string.h>

void test_parse_pat_input(void);
void test_parse_pat_data(void);

int main(int argc, char **argv)
{
    test_parse_pat_input();
    test_parse_pat_data();
    return 0;
}

void test_parse_pat_input(void)
{
    test_start("tsd_parse_pat input");

    TSDemuxContext ctx;
    TSDPATData pat;
    TSDCode res;

    tsd_context_init(&ctx);
    memset(&pat, 0, sizeof(pat));

    uint8_t data[] = {
        0xAB, 0xCD, 0xE6, 0x69,
    };

    res = tsd_parse_pat(NULL, NULL, 0, NULL);
    test_assert_equal(TSD_INVALID_CONTEXT, res, "invalid context");
    res = tsd_parse_pat(&ctx, NULL, 0, NULL);
    test_assert_equal(TSD_INVALID_DATA, res, "invalid data");
    res = tsd_parse_pat(&ctx, data, 3, NULL);
    test_assert_equal(TSD_INVALID_DATA_SIZE, res, "invalid size");
    res = tsd_parse_pat(&ctx, data, sizeof(data), NULL);
    test_assert_equal(TSD_INVALID_ARGUMENT, res, "invalid argument");
    res = tsd_parse_pat(&ctx, data, sizeof(data), &pat);
    test_assert_equal(TSD_OK, res, "successful parse");
    test_assert_equal(1, pat.length, "length");
    test_assert_equal(0xABCD, pat.program_number[0], "program number");
    test_assert_equal(0x0669, pat.pid[0], "pid");

    test_end();
}

void test_parse_pat_data(void)
{
    test_start("tsd_parse_pat data");

    TSDemuxContext ctx;
    TSDPATData pat;
    TSDCode res;

    tsd_context_init(&ctx);
    memset(&pat, 0, sizeof(pat));

    uint8_t data[] = {
        0xFE, 0xDC, 0xF4, 0x96,
        0x98, 0x76, 0xE0, 0x01,
    };
    uint8_t data2[] = {
        0x12, 0x34, 0xE0, 0xBB,
    };

    res = tsd_parse_pat(&ctx, data, sizeof(data), &pat);
    test_assert_equal(TSD_OK, res, "successful parse");
    test_assert_equal(2, pat.length, "length");
    test_assert_equal(0xFEDC, pat.program_number[0], "program number 1");
    test_assert_equal(0x1496, pat.pid[0], "pid 1");
    test_assert_equal(0x9876, pat.program_number[1], "program number 2");
    test_assert_equal(0x0001, pat.pid[1], "pid 2");

    res = tsd_parse_pat(&ctx, data2, sizeof(data2), &pat);
    test_assert_equal(TSD_OK, res, "successful parse");
    test_assert_equal(3, pat.length, "length");
    test_assert_equal(0xFEDC, pat.program_number[0], "program number 1");
    test_assert_equal(0x1496, pat.pid[0], "pid 1");
    test_assert_equal(0x9876, pat.program_number[1], "program number 2");
    test_assert_equal(0x0001, pat.pid[1], "pid 2");
    test_assert_equal(0x1234, pat.program_number[2], "program number 3");
    test_assert_equal(0x00BB, pat.pid[2], "pid 3");

    test_end();
}
