#include "test.h"
#include <tsdemux.h>
#include <stdio.h>

void test_input(void);
void test_parsing(void);

int main() {
    test_input();
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

void test_parsing(void)
{
    test_start("parse_packet_header parsing");
    
    FILE *fp = fopen("test/data/00.ts", "rb");
    test_assert_null(fp, "open 'test/data/00.ts'");
    char *buffer[20480]; // 20KB
    size_t len = fread(buffer, 1, 20480, fp);

    TSDemuxContext ctx;
    TSPacket pkt;
    TSCode res = parse_packet_header(&ctx, buffer, len, &pkt);

    test_assert_equal(TSD_OK, res, "parsing should return TSD_OK");


    test_end();
}
