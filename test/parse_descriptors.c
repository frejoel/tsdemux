#include "test.h"
#include <tsdemux.h>
#include <stdio.h>

void test_video_stream(void);

int main(int argc, char **argv)
{
    test_video_stream();
    return 0;
}

void test_video_stream(void)
{
    test_start("video_stream_descriptor");
    
    TSDDescriptorVideoStream desc;
    const uint8_t data[] = {
        0x02, // tag
        0x03, // length
        0b10101011, // mulit frame rate flag (1), frame_rate_code(4), MPEG_1_only_flag (1), constrained_param (1), still pic(1)
        0x66, // profile level indicator
        0b01011111, // chroma(2), frame rate ext. flag(1), reserved(5)
    };

    TSDCode res;
    res = tsd_parse_descriptor_video_stream(NULL, sizeof(data), &desc);
    test_assert_equal(TSD_INVALID_DATA, res, "invalid data");
    res = tsd_parse_descriptor_video_stream(data, 2, &desc);
    test_assert_equal(TSD_INVALID_DATA_SIZE, res, "invalid data size");
    res = tsd_parse_descriptor_video_stream(data, sizeof(data), NULL);
    test_assert_equal(TSD_INVALID_ARGUMENT, res, "invalid argument");

    res = tsd_parse_descriptor_video_stream(data, sizeof(data), &desc);
    test_assert_equal(TSD_OK, res, "TSD_OK");
    test_assert_equal(0x02, desc.tag, "tag");
    test_assert_equal(0x03, desc.length, "length");
    test_assert_equal(TSD_DFVS_MULTI_FRAME_RATE, desc.flags & TSD_DFVS_MULTI_FRAME_RATE, "multi frame rate flag");
    test_assert_equal(0, desc.flags & TSD_DFVS_MPEG_1_ONLY, "mpeg 1 only flag");
    test_assert_equal(TSD_DFVS_CONSTRAINED_PARAM, desc.flags & TSD_DFVS_CONSTRAINED_PARAM, "constrained flag");
    test_assert_equal(TSD_DFVS_STILL_PIC, desc.flags & TSD_DFVS_STILL_PIC, "still picture flag");
    test_assert_equal(0x05, desc.frame_rate_code, "frame rate code");

    test_end();
}
