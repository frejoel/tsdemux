#include "test.h"
#include <tsdemux.h>
#include <stdio.h>

void test_input(void);
void test_parsing_sync_byte(void);
void test_parsing(void);

int main() {
    test_input();
    test_parsing_sync_byte();
    test_parsing();
    return 0;
}

void test_input(void) {
  test_start("parse_packet_header input");

  FILE *fp = fopen("test/data/00.ts", "rb");
  test_assert_null(fp, "open 'test/data/00.ts'");
  char *buffer[20480]; // 20KB
  size_t len = fread(buffer, 1, 20480, fp);

  TSDemuxContext ctx;
  TSPacket pkt;

  // test the input of arguments
  TSCode res;
  res = parse_packet_header(&ctx, buffer, len, &pkt);
  test_assert_equal(res, TSD_OK, "parse_packet_header() should return TSD_OK");

  res = parse_packet_header(&ctx, NULL, 0, &pkt);
  test_assert_equal(TSD_INVALID_DATA, res, "parse_packet_header() should return TSD_INVALID_DATA");

  res = parse_packet_header(&ctx, buffer, 10, &pkt);
  test_assert_equal(TSD_INVALID_DATA_SIZE, res, "parse_packet_header() should return TSD_INVALID_DATA_SIZE");

  res = parse_packet_header(NULL, buffer, len, &pkt);
  test_assert_equal(TSD_INVALID_CONTEXT, res, "parse_packet_header() should return TSD_INVALID_CONTEXT");

  res = parse_packet_header(&ctx, buffer, len, NULL);
  test_assert_equal(TSD_INVALID_ARGUMENT, res, "parse_packet_header() should return TSD_INVALID_ARGUMENT");

  test_end();
}

void test_parsing_sync_byte(void)
{
    test_start("parse packet header - bad sync byte");

    // parse a bad packet (bad sync byte)
    char bad_packet[188];
    bad_packet[0] = 'H';
    TSDemuxContext ctx;
    TSPacket pkt;
    TSCode res = parse_packet_header(&ctx, bad_packet, 188, &pkt);
    test_assert_equal(TSD_INVALID_SYNC_BYTE, res, "Invalid Sync Byte");

    test_end();
}

void test_parsing(void)
{
    test_start("parse_packet_header parsing");

    // parse a TS Packet
    FILE *fp = fopen("test/data/00.ts", "rb");
    test_assert_null(fp, "open 'test/data/00.ts'");
    char buffer[20480]; // 20KB
    size_t len = fread(buffer, 1, 20480, fp);

    TSDemuxContext ctx;
    TSPacket pkt;
    TSCode res = parse_packet_header(&ctx, buffer, len, &pkt);

    test_assert_equal(TSD_OK, res, "parsing should return TSD_OK");
    test_assert_equal(0x47, pkt.sync_byte, "valid sync byte");
    test_assert_equal(0, pkt.flags & TSPF_TRANSPORT_ERROR_INDICATOR, "transport error indicator");
    test_assert(pkt.flags & TSPF_PAYLOAD_UNIT_START_INDICATOR, "payload unit start indicator");
    test_assert_equal(0, pkt.flags & TSPF_TRANSPORT_PRIORITY, "transport priority flag");
    test_assert_equal(0x11, pkt.pid, "PID");
    test_assert_equal(SC_NO_SCRAMBLING, pkt.transport_scrambling_control, "transport scrambling control");
    test_assert_equal(AFC_NO_FIELD_PRESENT, pkt.adaptation_field_control, "adaptation field control");
    test_assert_equal(5, pkt.continuity_counter, "continuity counter");
    test_assert_equal_ptr((size_t)(buffer+4), (size_t)pkt.data_bytes, "the data starts at the end of the header");

    test_end();
}
