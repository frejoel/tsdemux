#include "test.h"
#include <tsdemux.h>
#include <string.h>

void parse_pmt_input(void);

int main(int argc, char **argv)
{
    parse_pmt_input();
    return 0;
}

void parse_pmt_input(void)
{
    test_start("parse pmt input");

    TSDemuxContext ctx;
    TSCode res;
    PMTData pmt;
    memset(&pmt, 0, sizeof(pmt));

    uint8_t data[] = {
        0x01, 0x02, 0x03, 0x04
    };

    res = parse_pmt(NULL, NULL, 0, NULL);
    test_assert_equal(TSD_INVALID_CONTEXT, res, "invalid context");
    res = parse_pmt(&ctx, NULL, 0, NULL);
    test_assert_equal(TSD_INVALID_DATA, res, "invalid data");
    res = parse_pmt(&ctx, data, 0, NULL);
    test_assert_equal(TSD_INVALID_DATA_SIZE, res, "invalid size");
    res = parse_pmt(&ctx, data, sizeof(data), NULL);
    test_assert_equal(TSD_INVALID_ARGUMENT, res, "invalid PMTData");

    test_end();
}
