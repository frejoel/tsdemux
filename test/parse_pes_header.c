#include "test.h"
#include <tsdemux.h>
#include <stdio.h>

void test_input(void);
void test_pes_parse(void);

int main(int argc, char **argv)
{
    test_input();
    test_pes_parse();
    return 0;
}

void test_input(void)
{
    test_start("parse_pes input");

    FILE *fp = fopen("test/data/00.ts", "rb");
    test_assert_null(fp, "open 'test/data/00.ts'");
    char *buffer[20480]; // 20KB
    size_t len = fread(buffer, 1, 20480, fp);
    char *pes_buffer = NULL;

    TSDemuxContext ctx;
    TSPacket ts;
    PESPacket pkt;
    TSCode res;

    // find the first PES
    size_t i;
    for(i=0; i<len; i+=188) {
        res = parse_packet_header(&ctx, &buffer[i], len-(i*188), &ts);
        test_assert(res, TSD_OK, "parsing TS packet to get a PES packet");

        if((ts.flags & TSPF_TRANSPORT_ERROR_INDICATOR) ||
            (ts.adaptation_field_control == AFC_ADAPTATION_FIELD_ONLY) ||
            (ts.adaptation_field_control == AFC_RESERVED))
        {
            // skip this packet
            continue;
        }
        // does this packet contain the first byte of the payload?
        if(ts.flags & TSPF_PAYLOAD_UNIT_START_INDICATOR) {
            // is it a PES payload?
            pes_buffer = NULL;
        }
    }

    // test the input of arguments
    //res = parse_packet_header(&ctx, buffer, len, &pkt);
    //test_assert_equal(res, TSD_OK, "parse_packet_header() should return TSD_OK");

    res = parse_packet_header(&ctx, NULL, 0, &pkt);
    test_assert_equal(TSD_INVALID_DATA, res, "parse_packet_header() should return TSD_INVALID_DATA");

    res = parse_packet_header(&ctx, buffer, 5, &pkt);
    test_assert_equal(TSD_INVALID_DATA_SIZE, res, "parse_packet_header() should return TSD_INVALID_DATA_SIZE");

    res = parse_packet_header(NULL, buffer, len, &pkt);
    test_assert_equal(TSD_INVALID_CONTEXT, res, "parse_packet_header() should return TSD_INVALID_CONTEXT");

    res = parse_packet_header(&ctx, buffer, len, NULL);
    test_assert_equal(TSD_INVALID_ARGUMENT, res, "parse_packet_header() should return TSD_INVALID_ARGUMENT");

    test_end();
}

void test_pes_parse(void)
{

    test_start("pes parse input");

    FILE *fp = fopen("test/data/00.ts", "rb");
    test_assert_null(fp, "open 'test/data/00.ts'");
    char *buffer[20480]; // 20KB
    size_t len = fread(buffer, 1, 20480, fp);

    TSDemuxContext ctx;
    PESPacket pkt;

    // test the input of arguments
    TSCode res;
    res = parse_pes(&ctx, buffer, len, &pkt);

    test_end();
}
