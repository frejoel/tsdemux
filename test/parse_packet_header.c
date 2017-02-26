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
    TSDPacket pkt;

    // test the input of arguments
    TSDCode res;
    res = tsd_parse_packet_header(&ctx, buffer, len, &pkt);
    test_assert_equal(res, TSD_OK, "tsd_parse_packet_header() should return TSD_OK");

    res = tsd_parse_packet_header(&ctx, NULL, 0, &pkt);
    test_assert_equal(TSD_INVALID_DATA, res, "tsd_parse_packet_header() should return TSD_INVALID_DATA");

    res = tsd_parse_packet_header(&ctx, buffer, 10, &pkt);
    test_assert_equal(TSD_INVALID_DATA_SIZE, res, "tsd_parse_packet_header() should return TSD_INVALID_DATA_SIZE");

    res = tsd_parse_packet_header(NULL, buffer, len, &pkt);
    test_assert_equal(TSD_INVALID_CONTEXT, res, "tsd_parse_packet_header() should return TSD_INVALID_CONTEXT");

    res = tsd_parse_packet_header(&ctx, buffer, len, NULL);
    test_assert_equal(TSD_INVALID_ARGUMENT, res, "tsd_parse_packet_header() should return TSD_INVALID_ARGUMENT");

    test_end();
}

void test_parsing_sync_byte(void)
{
    test_start("parse packet header - bad sync byte");

    // parse a bad packet (bad sync byte)
    uint8_t bad_packet[188];
    bad_packet[0] = 'H';
    TSDemuxContext ctx;
    TSDPacket pkt;
    TSDCode res = tsd_parse_packet_header(&ctx, bad_packet, 188, &pkt);
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
    TSDPacket pkt;
    TSDCode res = tsd_parse_packet_header(&ctx, buffer, len, &pkt);

    test_assert_equal(TSD_OK, res, "parsing should return TSD_OK");
    test_assert_equal(0x47, pkt.sync_byte, "valid sync byte");
    test_assert_equal(0, pkt.flags & TSD_PF_TRAN_ERR_INDICATOR, "transport error indicator");
    test_assert(pkt.flags & TSD_PF_PAYLOAD_UNIT_START_IND, "payload unit start indicator");
    test_assert_equal(0, pkt.flags & TSD_PF_TRAN_PRIORITY, "transport priority flag");
    test_assert_equal(0x11, pkt.pid, "PID");
    test_assert_equal(TSD_SC_NO_SCRAMBLING, pkt.transport_scrambling_control, "transport scrambling control");
    test_assert_equal(TSD_AFC_NO_FIELD_PRESENT, pkt.adaptation_field_control, "adaptation field control");
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
    TSDPacket pkt;
    char *ptr = &buffer[188 * 3];
    TSDCode res = tsd_parse_packet_header(&ctx, ptr, len, &pkt);

    test_assert_equal(TSD_OK, res, "parsing should return TSD_OK");
    test_assert_equal(0x47, pkt.sync_byte, "valid sync byte");
    test_assert_equal(0, pkt.flags & TSD_PF_TRAN_ERR_INDICATOR, "transport error indicator");
    test_assert(pkt.flags & TSD_PF_PAYLOAD_UNIT_START_IND, "payload unit start indicator");
    test_assert_equal(0, pkt.flags & TSD_PF_TRAN_PRIORITY, "transport priority flag");
    test_assert_equal(0x100, pkt.pid, "PID");
    test_assert_equal(TSD_SC_NO_SCRAMBLING, pkt.transport_scrambling_control, "transport scrambling control");
    test_assert_equal(TSD_AFC_ADAP_FIELD_AND_PAYLOAD, pkt.adaptation_field_control, "adaptation field control");
    test_assert_equal(3, pkt.continuity_counter, "continuity counter");
    // starts at byte 568
    TSDAdaptationField *af = &pkt.adaptation_field;
    test_assert_equal(7, af->adaptation_field_length, "adaptation field length");
    test_assert_equal(0, af->flags & TSD_AF_DISCON_IND, "discontinuity indicator");
    test_assert_equal(TSD_AF_RANDOM_ACCESS_IND, af->flags & TSD_AF_RANDOM_ACCESS_IND, "random access indicator");
    test_assert_equal(0, af->flags & TSD_AF_ELEM_STREAM_PRIORIY_IND, "elementary stream priority indicator");
    test_assert_equal(TSD_AF_PCR_FLAG, af->flags & TSD_AF_PCR_FLAG, "PCR Flag");
    test_assert_equal(0, af->flags & TSD_AF_OPCR_FLAG, "OPCR Flag");
    test_assert_equal(0, af->flags & TSD_AF_SPLICING_POINT_FLAG, "Splicing Point Flag");
    test_assert_equal(0, af->flags & TSD_AF_ADAP_FIELD_EXT_FLAG, "Adaptation Filed Extension Flag");
    test_assert_equal(96008749L, af->program_clock_ref_base, "Program Clock Reference Base");
    test_assert_equal(0L, af->program_clock_ref_ext, "Program Clock Reference Extension");
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
    TSDPacket pkt;
    char *ptr = buffer;

    TSDCode res = tsd_parse_packet_header(&ctx, ptr, len, &pkt);
    test_assert_equal(TSD_OK, res, "should return TSDOK");
    test_assert_equal(0x47, pkt.sync_byte, "sync byte");
    test_assert_equal(0, pkt.flags & TSD_PF_TRAN_ERR_INDICATOR, "transport error indicator");
    test_assert_equal(0, pkt.flags & TSD_PF_PAYLOAD_UNIT_START_IND, "payload unit start indicator");
    test_assert_equal(0, pkt.flags & TSD_PF_TRAN_PRIORITY, "transport priority");
    test_assert_equal(0x400, pkt.pid, "PID");
    test_assert_equal(TSD_SC_NO_SCRAMBLING, pkt.transport_scrambling_control, "transport scrambling controle");
    test_assert_equal(TSD_AFC_ADAP_FIELD_ONLY, pkt.adaptation_field_control, "adaptation field control");
    test_assert_equal(10, pkt.continuity_counter, "continuity counter");
    test_assert_equal(0, (size_t)pkt.data_bytes, "data bytes");
    test_assert_equal(0, pkt.data_bytes_length, "the length of the byte data");

    TSDAdaptationField *af = &pkt.adaptation_field;
    test_assert_equal(0x25, af->adaptation_field_length, "adaptation field length");
    test_assert_equal(0, af->flags & TSD_AF_DISCON_IND, "discontinuity indicator");
    test_assert_equal(0, af->flags & TSD_AF_RANDOM_ACCESS_IND, "random access indicator");
    test_assert_equal(0, af->flags & TSD_AF_ELEM_STREAM_PRIORIY_IND, "elementary stream priority indicator");
    test_assert_equal(TSD_AF_PCR_FLAG, af->flags & TSD_AF_PCR_FLAG, "PCR flag");
    test_assert_equal(TSD_AF_OPCR_FLAG, af->flags & TSD_AF_OPCR_FLAG, "OPCR flag");
    test_assert_equal(TSD_AF_SPLICING_POINT_FLAG, af->flags & TSD_AF_SPLICING_POINT_FLAG, "Slicing point flag");
    test_assert_equal(TSD_AF_TRAN_PRIVATE_DATA_FLAG, af->flags & TSD_AF_TRAN_PRIVATE_DATA_FLAG, "transport private data flag");
    test_assert_equal(TSD_AF_ADAP_FIELD_EXT_FLAG, af->flags & TSD_AF_ADAP_FIELD_EXT_FLAG, "adaptation field extension flag");

    test_assert_equal_uint64(0x13F3D4968L, af->program_clock_ref_base, "program clock reference base");
    test_assert_equal(0xF7, af->program_clock_ref_ext, "program clock reference extension");
    test_assert_equal_uint64(0x10750337EL, af->orig_program_clock_ref_base, "original program clock reference base");
    test_assert_equal(0xF2, af->orig_program_clock_ref_ext, "original program clock reference extension");
    test_assert_equal(0x03, af->splice_countdown, "splice countdown");
    test_assert_equal(0x03, af->transport_private_data_length, "transport private data length");
    test_assert_equal_ptr((size_t)(&buffer[20]), (size_t)af->private_data_byte, "private data byte");

    TSDAdaptationFieldExtension *ae = &af->adap_field_ext;
    test_assert_equal(0xE0, ae->length, "adaptation field extension length");
    test_assert_equal(TSD_AFEF_LTW_FLAG, ae->flags & TSD_AFEF_LTW_FLAG, "ltw flag");
    test_assert_equal(TSD_AFEF_PIECEWISE_RATE_FLAG, ae->flags & TSD_AFEF_PIECEWISE_RATE_FLAG, "piecewise rate flag");
    test_assert_equal(TSD_AFEF_SEAMLESS_SPLCE_FLAG, ae->flags & TSD_AFEF_SEAMLESS_SPLCE_FLAG, "seamless splice flag");
    test_assert_equal(0x565E, ae->ltw_offset, "ltw offset");
    test_assert_equal(0x200EF0, ae->piecewise_rate, "piecewise rate");
    test_assert_equal_uint64(0xFF606ED2L, ae->dts_next_au, "DTS next AU");

    test_end();
}
