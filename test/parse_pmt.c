#include "test.h"
#include <tsdemux.h>
#include <string.h>

void parse_pmt_input(void);
void parse_pmt_data(void);
void parse_pmt_empty_data(void);

int main(int argc, char **argv)
{
    parse_pmt_input();
    parse_pmt_data();
    parse_pmt_empty_data();
    return 0;
}

void parse_pmt_input(void)
{
    test_start("parse pmt input");

    TSDemuxContext ctx;
    TSDCode res;
    TSDPMTData pmt;
    memset(&pmt, 0, sizeof(pmt));

    uint8_t data[] = {
        0x01, 0x02, 0x03, 0x04
    };

    res = tsd_parse_pmt(NULL, NULL, 0, NULL);
    test_assert_equal(TSD_INVALID_CONTEXT, res, "invalid context");
    res = tsd_parse_pmt(&ctx, NULL, 0, NULL);
    test_assert_equal(TSD_INVALID_DATA, res, "invalid data");
    res = tsd_parse_pmt(&ctx, data, 0, NULL);
    test_assert_equal(TSD_INVALID_DATA_SIZE, res, "invalid size");
    res = tsd_parse_pmt(&ctx, data, sizeof(data), NULL);
    test_assert_equal(TSD_INVALID_ARGUMENT, res, "invalid TSDPMTData");

    test_end();
}

void parse_pmt_data(void)
{
    test_start("parse pmt data");

    TSDemuxContext ctx;
    TSDPMTData pmt;
    TSDCode res;

    tsd_context_init(&ctx);
    memset(&pmt, 0, sizeof(pmt));

    uint8_t data[] = {
        0xE0, 0x99, // PCR PID = 0x99
        0xF0, 0x09, // program info length = 9
        0x9A, 0x03, 0xFA, 0xFA, 0xFA, // program descriptor
        0x8B, 0x02, 0xBB, 0xCC, // program descriptor
        0xEF, // stream type
        0xE0, 0x3E, // elementary PID
        0xF0, 0x03, // ES Info Length = 3
        0x11, 0x01, 0x12, // descriptor
        0x44, // stream type
        0xE1, 0x51, // elementary PID
        0xF0, 0x0A, // ES Info Length = 10
        0x19, 0x03, 0x2B, 0x45, 0xFE, // descriptor
        0x18, 0x03, 0xB2, 0x54, 0xEF, // descriptor
        0xCC, 0xCC, 0xEE, 0x54, // CRC32
    };

    res = tsd_parse_pmt(&ctx, data, sizeof(data), &pmt);
    test_assert_equal(TSD_OK, res, "parse valid data");
    test_assert_equal(0x99, pmt.pcr_pid, "PCR PID");
    test_assert_equal(0x09, pmt.program_info_length, "program info length");
    test_assert_equal(2, pmt.descriptors_length, "descriptors length");
    test_assert_equal(0x9A, pmt.descriptors[0].tag, "descriptor tag 1");
    test_assert_equal(0x03, pmt.descriptors[0].length, "descriptor length 1");
    test_assert_equal(0x05, pmt.descriptors[0].data_length, "descriptor data length 1");
    test_assert_equal(memcmp(pmt.descriptors[0].data, &data[4], 3), 0, "descriptor data 1");
    test_assert_equal(0x8B, pmt.descriptors[1].tag, "descriptor tag 2");
    test_assert_equal(0x02, pmt.descriptors[1].length, "descriptor length 2");
    test_assert_equal(0x04, pmt.descriptors[1].data_length, "descriptor data length 2");
    test_assert_equal(memcmp(pmt.descriptors[1].data, &data[9], 2), 0, "descriptor data 2");
    test_assert_equal(2, pmt.program_elements_length, "program elements length");

    test_assert_equal(0xEF, pmt.program_elements[0].stream_type, "stream tpye 1");
    test_assert_equal(0x003E, pmt.program_elements[0].elementary_pid, "elementary pid 1");
    test_assert_equal(0x03, pmt.program_elements[0].es_info_length, "es info lenth 1");
    test_assert_equal(1, pmt.program_elements[0].descriptors_length, "program elements descriptor length 1");
    test_assert_equal(0x11, pmt.program_elements[0].descriptors[0].tag, "program elements descriptor tag 1");
    test_assert_equal(1, pmt.program_elements[0].descriptors[0].length, "program elements descriptor length 1");
    test_assert_equal(3, pmt.program_elements[0].descriptors[0].data_length, "program elements descriptor data length 1");
    test_assert_equal(memcmp(pmt.program_elements[0].descriptors[0].data, &data[18], 1), 0, "program element descriptor data 1");

    test_assert_equal(0x44, pmt.program_elements[1].stream_type, "stream type 1");
    test_assert_equal(0x0151, pmt.program_elements[1].elementary_pid, "elementary pid 1");
    test_assert_equal(0x0A, pmt.program_elements[1].es_info_length, "es info lenth 1");
    test_assert_equal(2, pmt.program_elements[1].descriptors_length, "program elements 2 descriptor length 1");
    test_assert_equal(0x19, pmt.program_elements[1].descriptors[0].tag, "program elements 2 descriptor tag 1");
    test_assert_equal(3, pmt.program_elements[1].descriptors[0].length, "program elements 2 descriptor length 1");
    test_assert_equal(5, pmt.program_elements[1].descriptors[0].data_length, "program elements 2 descriptor data length 1");
    test_assert_equal(memcmp(pmt.program_elements[1].descriptors[0].data, &data[26], 3), 0, "program element 2 descriptor data 1");
    test_assert_equal(0x18, pmt.program_elements[1].descriptors[1].tag, "program elements 2 descriptor tag 2");
    test_assert_equal(3, pmt.program_elements[1].descriptors[1].length, "program elements 2 descriptor length 2");
    test_assert_equal(5, pmt.program_elements[1].descriptors[1].data_length, "program elements 2 descriptor data length 2");
    test_assert_equal(memcmp(pmt.program_elements[1].descriptors[1].data, &data[31], 3), 0, "program element 2 descriptor data 2");

    test_assert_equal(0xCCCCEE54, pmt.crc_32, "crc32");

    test_end();
}

void parse_pmt_empty_data(void)
{
    test_start("parse pmt empty data");

    TSDemuxContext ctx;
    TSDPMTData pmt;
    TSDCode res;

    tsd_context_init(&ctx);
    memset(&pmt, 0, sizeof(pmt));

    uint8_t data[] = {
        0xE0, 0x99, // PCR PID = 0x99
        0xF0, 0x00, // program info length = 9
        0xEF, // stream type
        0xE0, 0x3E, // elementary PID
        0xF0, 0x00, // ES Info Length = 0
        0x44, // stream type
        0xE1, 0x51, // elementary PID
        0xF0, 0x00, // ES Info Length = 10
        0xCC, 0xCC, 0xEE, 0x54, // CRC32
    };

    res = tsd_parse_pmt(&ctx, data, sizeof(data), &pmt);
    test_assert_equal(TSD_OK, res, "parse valid data");
    test_assert_equal(0x99, pmt.pcr_pid, "PCR PID");
    test_assert_equal(0x00, pmt.program_info_length, "program info length");
    test_assert_equal(0, pmt.descriptors_length, "descriptors length");
    test_assert_equal(2, pmt.program_elements_length, "program elements length");
    test_assert_equal(0xEF, pmt.program_elements[0].stream_type, "stream tpye 1");
    test_assert_equal(0x003E, pmt.program_elements[0].elementary_pid, "elementary pid 1");
    test_assert_equal(0x00, pmt.program_elements[0].es_info_length, "es info lenth 1");
    test_assert_equal(0, pmt.program_elements[0].descriptors_length, "program elements descriptor length 1");
    test_assert_equal(0x44, pmt.program_elements[1].stream_type, "stream type 1");
    test_assert_equal(0x0151, pmt.program_elements[1].elementary_pid, "elementary pid 1");
    test_assert_equal(0x00, pmt.program_elements[1].es_info_length, "es info lenth 1");
    test_assert_equal(0, pmt.program_elements[1].descriptors_length, "program elements 2 descriptor length 1");
    test_assert_equal(0xCCCCEE54, pmt.crc_32, "crc32");

    test_end();
}
