#include "test.h"
#include <tsdemux.h>
#include <string.h>

void test_parse_cat_input(void);
void test_parse_cat_data(void);

int main(int argc, char **argv)
{
    test_parse_cat_input();
    test_parse_cat_data();
    return 0;
}

void test_parse_cat_input(void)
{
    test_start("parse_cat input");

    TSDemuxContext ctx;
    TSDCATData cat;
    TSDCode res;

    tsd_context_init(&ctx);
    memset(&cat, 0, sizeof(cat));

    uint8_t data[] = {
        0xAB, 0x02, 0xE6, 0x69,
    };

    res = tsd_parse_descriptors(NULL, NULL, 0, NULL);
    test_assert_equal(TSD_INVALID_CONTEXT, res, "invalid context");
    res = tsd_parse_descriptors(&ctx, NULL, 0, NULL);
    test_assert_equal(TSD_INVALID_DATA, res, "invalid data");
    res = tsd_parse_descriptors(&ctx, data, 0, NULL);
    test_assert_equal(TSD_INVALID_DATA_SIZE, res, "invalid size");
    res = tsd_parse_descriptors(&ctx, data, sizeof(data), NULL);
    test_assert_equal(TSD_INVALID_ARGUMENT, res, "invalid argument");
    res = tsd_parse_descriptors(&ctx, data, sizeof(data), &cat);
    test_assert_equal(TSD_OK, res, "successful parse");
    test_assert_equal(1, cat.descriptors_length, "descriptor length");
    test_assert(cat.descriptors != NULL, "valid descriptors");

    test_end();
}

void test_parse_cat_data(void)
{
    test_start("parse_cat data");

    TSDemuxContext ctx;
    TSDCATData cat;
    TSDCode res;

    tsd_context_init(&ctx);
    memset(&cat, 0, sizeof(cat));

    uint8_t data[] = {
        0xFE, 0x03, 0xF4, 0x96, 0xFF,
        0x98, 0x04, 0xE0, 0x01, 0xFF, 0xFC,
    };

    res = tsd_parse_descriptors(&ctx, data, sizeof(data), &cat);
    test_assert_equal(TSD_OK, res, "successful parse");
    test_assert_equal(2, cat.descriptors_length, "descriptor length");
    test_assert(cat.descriptors != NULL, "valid descriptors");

    test_end();
}
