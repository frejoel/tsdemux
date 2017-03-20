#include "test.h"
#include <tsdemux.h>
#include <stdio.h>

void test_video_stream(void);
void test_audio_stream(void);
void test_hierarchy(void);

int main(int argc, char **argv)
{
    test_video_stream();
    test_audio_stream();
    test_hierarchy();
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

void test_audio_stream(void)
{
    test_start("audio_stream_descriptor");

    TSDDescriptorAudioStream desc;
    const uint8_t data[] = {
        0x03, // tag
        0x01, // length
        0b10100111, // free format flag(1), ID(1), layer(2), variable rate audio indicator(1), reserved(3)
    };

    TSDCode res;
    res = tsd_parse_descriptor_audio_stream(NULL, sizeof(data), &desc);
    test_assert_equal(TSD_INVALID_DATA, res, "invalid data");
    res = tsd_parse_descriptor_audio_stream(data, 2, &desc);
    test_assert_equal(TSD_INVALID_DATA_SIZE, res, "invalid data size");
    res = tsd_parse_descriptor_audio_stream(data, sizeof(data), NULL);
    test_assert_equal(TSD_INVALID_ARGUMENT, res, "invalid argument");

    res = tsd_parse_descriptor_audio_stream(data, sizeof(data), &desc);
    test_assert_equal(TSD_OK, res, "TSD_OK");
    test_assert_equal(0x03, desc.tag, "tag");
    test_assert_equal(0x01, desc.length, "length");
    test_assert_equal(desc.flags & TSD_DFAS_FREE_FORMAT, TSD_DFAS_FREE_FORMAT, "free format");
    test_assert_equal(desc.flags & TSD_DFAS_ID, 0, "ID");
    test_assert_equal(desc.flags & TSD_DFAS_VAR_RATE_AUDIO_IND, 0, "variable rate audio indicator");
    test_assert_equal(desc.layer, 0x02, "layer");
    test_end();
}

void test_hierarchy(void)
{
    test_start("hierarchy_descriptor");

    TSDDescriptorHierarchy desc;
    const uint8_t data[] = {
        0x04, // tag
        0x04, // length
        0xF3, // reserved(4), type(4)
        0xD9, // resvered(2), layer index(6)
        0xE8, // reserved(2), embedded layer index(6)
        0xF7, // reserved(2), channel(6)
    };

    TSDCode res;
    res = tsd_parse_descriptor_hierarchy(NULL, sizeof(data), &desc);
    test_assert_equal(TSD_INVALID_DATA, res, "invalid data");
    res = tsd_parse_descriptor_hierarchy(data, 2, &desc);
    test_assert_equal(TSD_INVALID_DATA_SIZE, res, "invalid data size");
    res = tsd_parse_descriptor_hierarchy(data, sizeof(data), NULL);
    test_assert_equal(TSD_INVALID_ARGUMENT, res, "invalid argument");

    res = tsd_parse_descriptor_hierarchy(data, sizeof(data), &desc);
    test_assert_equal(TSD_OK, res, "TSD_OK");
    test_assert_equal(desc.tag, 0x04, "tag");
    test_assert_equal(desc.length, 0x04, "length");
    test_assert_equal(desc.type, 0x03, "type");
    test_assert_equal(desc.layer_index, 0x19, "layer index");
    test_assert_equal(desc.embedded_layer_index, 0x28, "embedded layer index");
    test_assert_equal(desc.channel, 0x37, "channel");
    test_end();
}
