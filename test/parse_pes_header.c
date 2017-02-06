#include "test.h"
#include <tsdemux.h>
#include <stdio.h>

void test_parse_pes_input(void);
void test_parse_pes_data(void);

int main(int argc, char **argv)
{
    test_parse_pes_input();
    test_parse_pes_data();
    return 0;
}

void test_parse_pes_input(void)
{
    test_start("parse_pes input");

    TSDemuxContext ctx;
    PESPacket pkt;
    TSCode res;

    set_default_context(&ctx);

    char buffer[] = {
        0x00, 0x00, 0x01, // start code prefix
        0x00,   // stream id
        0x00, 0x02, // packet length
        0xFF, 0xFF,
    };

    res = parse_pes(NULL, NULL, 0, NULL);
    test_assert_equal(TSD_INVALID_CONTEXT, res, "invalid context");
    res = parse_pes(&ctx, NULL, 0, &pkt);
    test_assert_equal(TSD_INVALID_DATA, res, "invalid data");
    res = parse_pes(&ctx, buffer, 5, &pkt);
    test_assert_equal(TSD_INVALID_DATA_SIZE, res, "invalid data size");
    res = parse_pes(&ctx, buffer, sizeof(buffer), NULL);
    test_assert_equal(TSD_INVALID_ARGUMENT, res, "invlaid PESPacket");

    test_end();
}

void test_parse_pes_data(void)
{

    test_start("parse_pes data");

    test_end();
}
