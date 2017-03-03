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
    test_start("tsd_parse_pes input");

    TSDemuxContext ctx;
    TSDPESPacket pkt;
    TSDCode res;

    tsd_set_default_context(&ctx);

    char buffer[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, };

    res = tsd_parse_pes(NULL, NULL, 0, NULL);
    test_assert_equal(TSD_INVALID_CONTEXT, res, "invalid context");
    res = tsd_parse_pes(&ctx, NULL, 0, &pkt);
    test_assert_equal(TSD_INVALID_DATA, res, "invalid data");
    res = tsd_parse_pes(&ctx, buffer, 5, &pkt);
    test_assert_equal(TSD_INVALID_DATA_SIZE, res, "invalid data size");
    res = tsd_parse_pes(&ctx, buffer, sizeof(buffer), NULL);
    test_assert_equal(TSD_INVALID_ARGUMENT, res, "invlaid TSDPESPacket");

    TSDPESPacket pes;
    res = tsd_parse_pes(&ctx, buffer, sizeof(buffer), &pes);
    test_assert_not_equal(TSD_OK, res, "invalid pes data");

    test_end();
}

void test_parse_pes_data(void)
{
    test_start("tsd_parse_pes data");

    TSDemuxContext ctx;
    TSDPESPacket pkt;
    TSDCode res;

    tsd_set_default_context(&ctx);

    char buffer[] = {
        0x00, 0x00, 0x01, // start code prefix
        0xE0,   // stream id
        0x00, 0x3C, // packet length
        0b10000010, // '10', scrmabling(2), priority(1), data alignment(1), copyright(1), original or copy(1)
        0b11101011, // pts dts(2), escr(1), es rate(1), dsm trick play(1), add copy info(1), crc (1), ext (1)
        0x34, // pes header data length
        0b00100001, 0x00, 0x01, 0x00, 0x01, // '0010', pts 4,3,1,15,1,15,1
        0b00011001, 0x00, 0x01, 0x00, 0x01, // '0001', dts 4,3,1,15,1,15,1
        0b11000100, 0x00, 0b00000100, 0x00, 0b00000100, 0x01, // ESCR, reserved(2), 3,1,15,1,15,1 ext 9,1
        0b01101000, // dsm trick play mode(3), field id(2), intra slice(1), freq trunc(2)
        0xAB, 0xBA, // crc flag
        // Extension
        0b01101111, //flags
        0x19, // Pack header length
            // Pack header
            0x00, 0x00, 0x01, 0xBA, // pack start code
            0b01000100, // '01' scr base(3), marker(1), scr(2)
            0x00, 0b00000100, // scr(8), scr(5), marker(1), scr(2)
            0x00, 0b00000100, // scr(8), scr(5), marker(1), scr ext(2)
            0x01, // scr ext(7), marker(1)
            0xAB, 0xCD, 0xE3, // mux rate(22), marker(2)
            0b11111011, // reserved(5), pack stuffing length(3)
            0xFF, 0xFF, 0xFF, // stuffing bytes
                // system headers
                0x00, 0x00, 0x01, 0xBB, // sys header start code
                0x00, 0x0C, // header length
                0b10000000, 0x00, 0b00000001, // marker(1), rate bound(22), marker(1)
                0b11110010, // audio bound(6), fixed(1), csps(1)
                0b10100110, // sys audio lock(1), sys video lock(1), marker(1), video bound(5)
                0b11111111, // packet rate restriction(1), reserved(7)
                    0b11110000, // stream id
                    0b11100000, 0xBC, // '11', buffer bound scale(1), buffer bound size(13)
                    0b11000001, // stream id
                    0b11010001, 0x04, // '11', buffer bound scale(1), buffer bound size(13)
            // Pack header end
        0b10001101, 0b11000011, // marker(1), packet seq counter(7), marker(1), mpeg1/2 id(1), orig stuff len(6)
        0b10000010, // marker(1), pes ext field length(7)
        0xFF, 0xFF, // reserved (pes ext field)
        0xFF, 0xFF,// stuffing bytes
        0xAB, 0xBC, 0xDE, 0xF1, 0x23, // pes packet bytes
    };

    TSDPESPacket pes;
    res = tsd_parse_pes(&ctx, buffer, sizeof(buffer), &pes);
    test_assert_equal(TSD_OK, res, "valid pes parsing");

    test_assert_equal(pes.start_code_prefix, 0x01, "pes start code prefix");
    test_assert_equal(pes.stream_id, 0xE0, "stream id");
    test_assert_equal(pes.packet_length, 0x3C, "packet length");
    test_assert_equal(pes.scrambling_control, 0x00, "scrambling control");
    test_assert_equal(pes.flags & TSD_PPF_PES_PRIORITY, 0x00, "priority");
    test_assert_equal(pes.flags & TSD_PPF_DATA_ALIGNMENT_INDICATOR, 0x00, "data alignment");
    test_assert_equal(pes.flags & TSD_PPF_COPYRIGHT, TSD_PPF_COPYRIGHT, "copyright");
    test_assert_equal(pes.flags & TSD_PPF_ORIGINAL_OR_COPY, 0x00, "original or copy");
    test_assert_equal(pes.flags & TSD_PPF_PTS_FLAG, TSD_PPF_PTS_FLAG, "pts flag");
    test_assert_equal(pes.flags & TSD_PPF_DTS_FLAG, TSD_PPF_DTS_FLAG, "dts flag");
    test_assert_equal(pes.flags & TSD_PPF_ESCR_FLAG, TSD_PPF_ESCR_FLAG, "escr flag");
    test_assert_equal(pes.flags & TSD_PPF_ES_RATE_FLAG, 0x00, "es rate flag");
    test_assert_equal(pes.flags & TSD_PPF_DSM_TRICK_MODE_FLAG, TSD_PPF_DSM_TRICK_MODE_FLAG, "dsm trick play");
    test_assert_equal(pes.flags & TSD_PPF_ADDITIONAL_COPY_INFO_FLAG, 0x00, "additional copy info flag");
    test_assert_equal(pes.flags & TSD_PPF_PES_CRC_FLAG, TSD_PPF_PES_CRC_FLAG, "CRC flag");
    test_assert_equal(pes.flags & TSD_PPF_PES_EXTENSION_FLAG, TSD_PPF_PES_EXTENSION_FLAG, "PES extension flag");
    test_assert_equal(pes.header_data_length, 0x34, "header data length");
    test_assert_equal(pes.pts, 0x00000000, "pts");
    test_assert_equal(pes.dts, 0x00000000, "dts");
    test_assert_equal(pes.escr, 0x00000000, "escr");
    test_assert_equal(pes.escr_extension, 0x00000000, "escr extension");
    test_assert_equal(pes.trick_mode.control, TSD_TMC_FAST_REVERSE, "trick mode control");
    test_assert_equal(pes.trick_mode.field_id, 0x01, "field id");
    test_assert_equal(pes.trick_mode.intra_slice_refresh, 0x00, "intra slice refresh");
    test_assert_equal(pes.trick_mode.frequency_truncation, 0x00, "frequency truncation");
    //test_assert_equal(pes., 0x, "");

    test_end();
}
