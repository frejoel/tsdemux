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

    uint8_t buffer[] = {
        0x47, // sync byte
        0b01000000, 0x00, // err ind(1), payload start(1), trans priority(1), PID(5+8=13)
        0b00010001, // scrambling(2), adapation field(2), cont. counter(4)
        // data bytes (12 per line) - stuffing
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF
    };
    size_t len = sizeof(buffer);

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
    uint8_t buffer[] = {
        0x47, // sync byte
        0b01000000, 0x11, // err ind(1), payload start(1), trans priority(1), PID(5+8=13)
        0b00010101, // scrambling(2), adapation field(2), cont. counter(4)
        // data bytes (12 per line) - stuffing
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF
    };
    size_t len = sizeof(buffer);

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
    uint8_t buffer[] = {
        0x47, // sync byte
        0b01000001, 0x00, // err ind(1), payload start(1), trans priority(1), PID(5+8=13)
        0b00110011, // scrambling(2), adapation field(2), cont. counter(4)
        // adaptation field
        0x07, // length
        0b01010000, // discon(1), randon acc(1), elem stream priority(1), pcr(1), opcr(1), splicing(1), private(1), ext(1)
        0x02, 0xDC, 0x7D, 0x16, 0x18, 0x00, // PCR(33), reserved(6), ext(9)
        // data bytes (12 per line) - stuffing
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    };
    size_t len = sizeof(buffer);

    TSDemuxContext ctx;
    TSDPacket pkt;
    char *ptr = buffer;
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

    TSDAdaptationField *af = &pkt.adaptation_field;
    test_assert_equal(7, af->adaptation_field_length, "adaptation field length");
    test_assert_equal(0, af->flags & TSD_AF_DISCON_IND, "discontinuity indicator");
    test_assert_equal(TSD_AF_RANDOM_ACCESS_IND, af->flags & TSD_AF_RANDOM_ACCESS_IND, "random access indicator");
    test_assert_equal(0, af->flags & TSD_AF_ELEM_STREAM_PRIORIY_IND, "elementary stream priority indicator");
    test_assert_equal(TSD_AF_PCR_FLAG, af->flags & TSD_AF_PCR_FLAG, "PCR Flag");
    test_assert_equal(0, af->flags & TSD_AF_OPCR_FLAG, "OPCR Flag");
    test_assert_equal(0, af->flags & TSD_AF_SPLICING_POINT_FLAG, "Splicing Point Flag");
    test_assert_equal(0, af->flags & TSD_AF_ADAP_FIELD_EXT_FLAG, "Adaptation Filed Extension Flag");
    test_assert_equal(96008748L, af->program_clock_ref_base, "Program Clock Reference Base");
    test_assert_equal(0L, af->program_clock_ref_ext, "Program Clock Reference Extension");
    test_assert_equal_ptr((size_t)(ptr+12), (size_t)pkt.data_bytes, "the data starts at the end of the adaptation field");
    test_assert_equal(176, pkt.data_bytes_length, "the length of the byte data");

    test_end();
}

void test_parsing_opcr(void)
{
    test_start("parse_packet_header OPCR parsing");

    // parse a TS Packet
    uint8_t buffer[] = {
        0x47, // sync byte
        0b00000100, 0x00, // err ind(1), payload start(1), trans priority(1), PID(5+8=13)
        0b00101010, // scrambling(2), adapation field(2), cont. counter(4)
        // adaptation field
        0x25, // length
        0b00011111, // discon(1), randon acc(1), elem stream priority(1), pcr(1), opcr(1), splicing(1), private(1), ext(1)
        0x9F, 0x9E, 0xA4, 0xB4, 0x00, 0xF7, // PCR(33), reserved(6), ext(9)
        0x83, 0xA8, 0x19, 0xBF, 0x00, 0xF2, // OPCR(33), reserved(6), ext(9)
        0x03, // splice countdown (8)
        0x03, // transport private data length(8)
        0xFF, 0xFF, 0xFF, // private data
        0xE0, // adaption field ext. length
        0xFF, // ltw(1), piecewise rate(1), seamless splice(1), reserved(5)
        0xD6, 0x5E, // ltw flag(1), ltw offset(15)
        0x20, 0x0E, 0xF0, // reserved(2), piecewie(22)
        // 0xFF606ED2
        // 1111 1111 0110 0000 0110 1110 1101 0010
        // 0000 0 11 1 11 1111 0110 0000 0 1 110 1110 1101 0010 1
        // 0000 0111 1111 1101 1000 0001 1101 1101 1010 0101
        0x07, 0xFD, 0x81, 0xDD, 0xA5, // splice type(4), DTS Next AU (3,1,15,1,15,1)
        // 35
        // data bytes (12 per line) - stuffing
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    };
    size_t len = sizeof(buffer);

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
    test_assert_equal_ptr((size_t)(&buffer[20]), (size_t)af->private_data_bytes, "private data byte");

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
