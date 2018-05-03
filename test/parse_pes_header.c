#include "test.h"
#include <tsdemux.h>
#include <stdio.h>
#include <string.h>

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

    tsd_context_init(&ctx);

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

    tsd_context_init(&ctx);

    char buffer[] = {
        0x00, 0x00, 0x01, // start code prefix
        0xE0,   // stream id
        0x00, 0x3C, // packet length
        0b10000010, // '10', scrmabling(2), priority(1), data alignment(1), copyright(1), original or copy(1)
        0b11101011, // pts dts(2), escr(1), es rate(1), dsm trick play(1), add copy info(1), crc (1), ext (1)
        0x49, // pes header data length
        0b00100001, 0x00, 0x01, 0x00, 0x01, // '0010', pts 4,3,1,15,1,15,1
        0b00011001, 0x00, 0x01, 0x00, 0x01, // '0001', dts 4,3,1,15,1,15,1
        0b11000100, 0x00, 0b00000100, 0x00, 0b00000100, 0x01, // ESCR, reserved(2), 3,1,15,1,15,1 ext 9,1
        0b01101000, // dsm trick play mode(3), field id(2), intra slice(1), freq trunc(2)
        0xAB, 0xBA, // crc flag
        // Extension
        0b01101111, //flags ,prv data(1), pack header(1), prog pk counter(1), pstd buffer(1), reserved(3), pes ext.2(1)
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

    test_assert_equal(pes.start_code, 0x01, "pes start code prefix");
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
    test_assert_equal(pes.header_data_length, 0x49, "header data length");
    test_assert_equal(pes.pts, 0x00000000, "pts");
    test_assert_equal(pes.dts, 0x00000000, "dts");
    test_assert_equal(pes.escr, 0x00000000, "escr");
    test_assert_equal(pes.escr_extension, 0x00000000, "escr extension");
    test_assert_equal(pes.trick_mode.control, TSD_TMC_FAST_REVERSE, "trick mode control");
    test_assert_equal(pes.trick_mode.field_id, 0x01, "field id");
    test_assert_equal(pes.trick_mode.intra_slice_refresh, 0x00, "intra slice refresh");
    test_assert_equal(pes.trick_mode.frequency_truncation, 0x00, "frequency truncation");
    test_assert_equal(pes.previous_pes_packet_crc, 0xABBA, "crc");
    test_assert_equal(pes.extension.flags & TSD_PEF_PES_PRIVATE_DATA_FLAG, 0x00, "pes private data");
    test_assert_equal(pes.extension.flags & TSD_PEF_PACK_HEADER_FIELD_FLAG, TSD_PEF_PACK_HEADER_FIELD_FLAG, "pack header");
    test_assert_equal(pes.extension.flags & TSD_PEF_PROGRAM_PACKET_SEQUENCE_COUNTER_FLAG, TSD_PEF_PROGRAM_PACKET_SEQUENCE_COUNTER_FLAG, "program packet seq counter");
    test_assert_equal(pes.extension.flags & TSD_PEF_PSTD_BUFFER_FLAG, 0x00, "p-std buffer");
    test_assert_equal(pes.extension.flags & TSD_PEF_PES_EXTENSION_FLAG_2, TSD_PEF_PES_EXTENSION_FLAG_2, "ext. flag 2");
    test_assert_equal(pes.extension.pack_header.length, 0x19, "pack header length");
    test_assert_equal(pes.extension.pack_header.start_code, 0x1BA, "pack header start code");
    test_assert_equal(pes.extension.pack_header.system_clock_ref_base, 0x00, "system clock ref base");
    test_assert_equal(pes.extension.pack_header.system_clock_ref_ext, 0x00, "system clock red ext.");
    test_assert_equal(pes.extension.pack_header.program_mux_rate, 0x2AF378, "program mux rate");
    test_assert_equal(pes.extension.pack_header.stuffing_length, 0x03, "pack stuffing length");
    TSDSystemHeader *syshdr = &pes.extension.pack_header.system_header;
    test_assert_equal(syshdr->start_code, 0x1BB, "system header start code");
    test_assert_equal(syshdr->length, 0x0C, "system header length");
    test_assert_equal(syshdr->rate_bound, 0x00, "system header rate bound");
    test_assert_equal(syshdr->audio_bound, 0x3C, "system header audio bound");
    test_assert_equal(syshdr->flags & TSD_SHF_FIXED_FLAG, TSD_SHF_FIXED_FLAG, "system header fixed flag");
    test_assert_equal(syshdr->flags & TSD_SHF_CSPS_FLAG, 0x00, "system header cspd flag");
    test_assert_equal(syshdr->flags & TSD_SHF_SYSTEM_AUDIO_LOCK_FLAG, TSD_SHF_SYSTEM_AUDIO_LOCK_FLAG, "system header audio lock");
    test_assert_equal(syshdr->flags & TSD_SHF_SYSTEM_VIDEO_LOCK_FLAG, 0x00, "system header video lock");
    test_assert_equal(syshdr->video_bound, 0x06, "system header video bound");
    test_assert_equal(syshdr->flags & TSD_SHF_PACKET_RATE_RESTICTION_FLAG, TSD_SHF_PACKET_RATE_RESTICTION_FLAG, "system header packet rate restriction");
    test_assert_equal(syshdr->stream_count, 0x02, "system header stream count");
    test_assert_equal(syshdr->streams[0].stream_id, 0xF0, "system header stream id 0");
    test_assert_equal(syshdr->streams[0].pstd_buffer_bound_scale, 0x01, "system header bound scale 0");
    test_assert_equal(syshdr->streams[0].pstd_buffer_size_bound, 0xBC, "system header bound size 0");
    test_assert_equal(syshdr->streams[1].stream_id, 0xC1, "system header stream id 1");
    test_assert_equal(syshdr->streams[1].pstd_buffer_bound_scale, 0x00, "system header bound scale 1");
    test_assert_equal(syshdr->streams[1].pstd_buffer_size_bound, 0x1104, "system header bound size 1");
    test_assert_equal(pes.extension.program_packet_sequence_counter, 0x0D, "sequence counter");
    test_assert_equal(pes.extension.mpeg1_mpeg2_identifier, 0x01, "mpeg1 mpeg2 indentifier");
    test_assert_equal(pes.extension.original_stuff_length, 0x03, "original stuff length");
    test_assert_equal(pes.extension.pes_extension_field_length, 0x02, "pes extension field length");
    test_assert_equal(pes.data_bytes_length, 0x05, "pes data length");
    char tmp_data[] = {0xAB, 0xBC, 0xDE, 0xF1, 0x23};
    test_assert_equal(memcmp(pes.data_bytes, tmp_data, 5), 0x00, "pes data");

    test_end();
}
