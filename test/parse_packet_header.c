#include "test.h"
#include <tsdemux.h>
#include <stdio.h>

void test_input(void);
void test_parsing_sync_byte(void);
void test_parsing(void);
void test_parsing_adaptation_field(void);
void test_parsing_opcr(void);

int main(int argc, char **argv)
{
    test_input();
    test_parsing_sync_byte();
    test_parsing();
    test_parsing_adaptation_field();
    test_parsing_opcr();
    return 0;
}

void test_input(void)
{
    test_start("parse_packet_header input");

    FILE *fp = fopen("test/data/00.ts", "rb");
    test_assert_null(fp, "open 'test/data/00.ts'");
    uint8_t buffer[20480]; // 20KB
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
    uint8_t bad_packet[188];
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
    uint8_t buffer[20480]; // 20KB
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
    test_assert_equal(184, pkt.data_bytes_length, "the length of the byte data");

    test_end();
}

void test_parsing_adaptation_field(void) {
    test_start("parse_packet_header adaptation field parsing");

    // parse a TS Packet
    FILE *fp = fopen("test/data/00.ts", "rb");
    test_assert_null(fp, "open 'test/data/00.ts'");
    uint8_t buffer[20480]; // 20KB
    size_t len = fread(buffer, 1, 20480, fp);

    TSDemuxContext ctx;
    TSPacket pkt;
    char *ptr = &buffer[188 * 3];
    TSCode res = parse_packet_header(&ctx, ptr, len, &pkt);

    test_assert_equal(TSD_OK, res, "parsing should return TSD_OK");
    test_assert_equal(0x47, pkt.sync_byte, "valid sync byte");
    test_assert_equal(0, pkt.flags & TSPF_TRANSPORT_ERROR_INDICATOR, "transport error indicator");
    test_assert(pkt.flags & TSPF_PAYLOAD_UNIT_START_INDICATOR, "payload unit start indicator");
    test_assert_equal(0, pkt.flags & TSPF_TRANSPORT_PRIORITY, "transport priority flag");
    test_assert_equal(0x100, pkt.pid, "PID");
    test_assert_equal(SC_NO_SCRAMBLING, pkt.transport_scrambling_control, "transport scrambling control");
    test_assert_equal(AFC_ADAPTATION_FIELD_AND_PAYLOAD, pkt.adaptation_field_control, "adaptation field control");
    test_assert_equal(3, pkt.continuity_counter, "continuity counter");
    // starts at byte 568
    AdaptationField *af = &pkt.adaptation_field;
    test_assert_equal(7, af->adaptation_field_length, "adaptation field length");
    test_assert_equal(0, af->flags & AF_DISCONTINUITY_INDICATOR, "discontinuity indicator");
    test_assert_equal(AF_RANDOM_ACCESS_INDICATOR, af->flags & AF_RANDOM_ACCESS_INDICATOR, "random access indicator");
    test_assert_equal(0, af->flags & AF_ELEMENTARY_STREAM_PRIORIY_INDICATOR, "elementary stream priority indicator");
    test_assert_equal(AF_PCR_FLAG, af->flags & AF_PCR_FLAG, "PCR Flag");
    test_assert_equal(0, af->flags & AF_OPCR_FLAG, "OPCR Flag");
    test_assert_equal(0, af->flags & AF_SPLICING_POINT_FLAG, "Splicing Point Flag");
    test_assert_equal(0, af->flags & AF_ADAPTATION_FIELD_EXTENSION_FLAG, "Adaptation Filed Extension Flag");
    test_assert_equal(96008749L, af->program_clock_reference_base, "Program Clock Reference Base");
    test_assert_equal(0L, af->program_clock_reference_extension, "Program Clock Reference Extension");
    test_assert_equal_ptr((size_t)(ptr+12), (size_t)pkt.data_bytes, "the data starts at the end of the adaptation field");
    test_assert_equal(176, pkt.data_bytes_length, "the length of the byte data");

    test_end();
}

void test_parsing_opcr(void)
{
    test_start("parse_packet_header OPCR parsing");

    // parse a TS Packet
    FILE *fp = fopen("test/data/01.ts", "rb");
    test_assert_null(fp, "open 'test/data/01.ts'");
    uint8_t buffer[2048000]; // 20KB
    size_t len = fread(buffer, 1, 2048000, fp);

    TSDemuxContext ctx;
    TSPacket pkt;
    char *ptr = buffer;

    TSCode res = parse_packet_header(&ctx, ptr, len, &pkt);
    test_assert_equal(TSD_OK, res, "should return TSDOK");
    test_assert_equal(0x47, pkt.sync_byte, "sync byte");
    test_assert_equal(0, pkt.flags & TSPF_TRANSPORT_ERROR_INDICATOR, "transport error indicator");
    test_assert_equal(0, pkt.flags & TSPF_PAYLOAD_UNIT_START_INDICATOR, "payload unit start indicator");
    test_assert_equal(0, pkt.flags & TSPF_TRANSPORT_PRIORITY, "transport priority");
    test_assert_equal(0x400, pkt.pid, "PID");
    test_assert_equal(SC_NO_SCRAMBLING, pkt.transport_scrambling_control, "transport scrambling controle");
    test_assert_equal(AFC_ADAPTATION_FIELD_ONLY, pkt.adaptation_field_control, "adaptation field control");
    test_assert_equal(10, pkt.continuity_counter, "continuity counter");
    test_assert_equal(0, (size_t)pkt.data_bytes, "data bytes");
    test_assert_equal(0, pkt.data_bytes_length, "the length of the byte data");

    AdaptationField *af = &pkt.adaptation_field;
    test_assert_equal(0x25, af->adaptation_field_length, "adaptation field length");
    test_assert_equal(0, af->flags & AF_DISCONTINUITY_INDICATOR, "discontinuity indicator");
    test_assert_equal(0, af->flags & AF_RANDOM_ACCESS_INDICATOR, "random access indicator");
    test_assert_equal(0, af->flags & AF_ELEMENTARY_STREAM_PRIORIY_INDICATOR, "elementary stream priority indicator");
    test_assert_equal(AF_PCR_FLAG, af->flags & AF_PCR_FLAG, "PCR flag");
    test_assert_equal(AF_OPCR_FLAG, af->flags & AF_OPCR_FLAG, "OPCR flag");
    test_assert_equal(AF_SPLICING_POINT_FLAG, af->flags & AF_SPLICING_POINT_FLAG, "Slicing point flag");
    test_assert_equal(AF_TRANSPORT_PRIVATE_DATA_FLAG, af->flags & AF_TRANSPORT_PRIVATE_DATA_FLAG, "transport private data flag");
    test_assert_equal(AF_ADAPTATION_FIELD_EXTENSION_FLAG, af->flags & AF_ADAPTATION_FIELD_EXTENSION_FLAG, "adaptation field extension flag");

    test_assert_equal_uint64(0x13F3D4968L, af->program_clock_reference_base, "program clock reference base");
    test_assert_equal(0xF7, af->program_clock_reference_extension, "program clock reference extension");
    test_assert_equal_uint64(0x10750337EL, af->original_program_clock_reference_base, "original program clock reference base");
    test_assert_equal(0xF2, af->original_program_clock_reference_extension, "original program clock reference extension");
    test_assert_equal(0x03, af->splice_countdown, "splice countdown");
    test_assert_equal(0x03, af->transport_private_data_length, "transport private data length");
    test_assert_equal_ptr((size_t)(&buffer[20]), (size_t)af->private_data_byte, "private data byte");

    AdaptationFieldExtension *ae = &af->adaptation_field_extension;
    test_assert_equal(0xE0, ae->length, "adaptation field extension length");
    test_assert_equal(AFEF_LTW_FLAG, ae->flags & AFEF_LTW_FLAG, "ltw flag");
    test_assert_equal(AFEF_PIECEWISE_RATE_FLAG, ae->flags & AFEF_PIECEWISE_RATE_FLAG, "piecewise rate flag");
    test_assert_equal(AFEF_SEAMLESS_SPLCE_FLAG, ae->flags & AFEF_SEAMLESS_SPLCE_FLAG, "seamless splice flag");
    test_assert_equal(0x565E, ae->ltw_offset, "ltw offset");
    test_assert_equal(0x200EF0, ae->piecewise_rate, "piecewise rate");
    test_assert_equal_uint64(0xFF606ED2L, ae->dts_next_au, "DTS next AU");

    test_end();
}
