#include "test.h"
#include <tsdemux.h>
#include <stdio.h>

void test_video_stream(void);
void test_audio_stream(void);
void test_hierarchy(void);
void test_registration(void);
void test_data_stream_alignment(void);
void test_target_background_grid(void);
void test_video_window(void);
void test_conditional_access(void);
void test_iso_639_language(void);
void test_scd(void);
void test_multiplex_buffer_util(void);
void test_copyright(void);
void test_max_bitrate(void);
void test_private_data_indicator(void);
void test_smoothing_buffer(void);
void test_std(void);
void test_ibp(void);
void test_mpeg4_video(void);
void test_mpeg4_audio(void);
void test_iod(void);
void test_sl(void);
void test_fmc(void);
void test_external_es_id(void);
void test_mux_code(void);
void test_fmx_buffer_size(void);
void test_multiplex_buffer(void);

int main(int argc, char **argv)
{
    test_video_stream();
    test_audio_stream();
    test_hierarchy();
    test_registration();
    test_data_stream_alignment();
    test_target_background_grid();
    test_video_window();
    test_conditional_access();
    test_iso_639_language();
    test_scd();
    test_multiplex_buffer_util();
    test_copyright();
    test_max_bitrate();
    test_private_data_indicator();
    test_smoothing_buffer();
    test_std();
    test_ibp();
    test_mpeg4_video();
    test_mpeg4_audio();
    test_iod();
    test_sl();
    test_fmc();
    test_external_es_id();
    test_mux_code();
    test_fmx_buffer_size();
    test_multiplex_buffer();
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

void test_registration(void)
{
    test_start("registration_descriptor");

    TSDDescriptorRegistration desc;
    const uint8_t data[] = {
        0x05, // tag
        0x07, // length
        0xAD, 0x54, 0xF5, 0x1C, // format idenitifier (4)
        0xFC, 0xAF, 0xED // additional identifier info (3)
    };

    TSDCode res;
    res = tsd_parse_descriptor_registration(NULL, sizeof(data), &desc);
    test_assert_equal(TSD_INVALID_DATA, res, "invalid data");
    res = tsd_parse_descriptor_registration(data, 2, &desc);
    test_assert_equal(TSD_INVALID_DATA_SIZE, res, "invalid data size");
    res = tsd_parse_descriptor_registration(data, sizeof(data), NULL);
    test_assert_equal(TSD_INVALID_ARGUMENT, res, "invalid argument");

    res = tsd_parse_descriptor_registration(data, sizeof(data), &desc);
    test_assert_equal(TSD_OK, res, "TSD_OK");
    test_assert_equal(desc.tag, 0x05, "tag");
    test_assert_equal(desc.length, 0x07, "length");
    test_assert_equal(desc.format_identifier, 0xAD54F51C, "format identifier");
    test_assert_equal_ptr((size_t)desc.additional_id_info, (size_t)&data[6], "additional identifier info");
    test_assert_equal(desc.additional_id_info_length, 3, "additional identifier info length");
    test_end();
}

void test_data_stream_alignment(void)
{
    test_start("data_stream_descriptor");

    TSDDescriptorDataStreamAlignment desc;
    const uint8_t data[] = {
        0x06, // tag
        0x01, // length
        0x02, // type
    };

    TSDCode res;
    res = tsd_parse_descriptor_data_stream_alignment(NULL, sizeof(data), &desc);
    test_assert_equal(TSD_INVALID_DATA, res, "invalid data");
    res = tsd_parse_descriptor_data_stream_alignment(data, 2, &desc);
    test_assert_equal(TSD_INVALID_DATA_SIZE, res, "invalid data size");
    res = tsd_parse_descriptor_data_stream_alignment(data, sizeof(data), NULL);
    test_assert_equal(TSD_INVALID_ARGUMENT, res, "invalid argument");

    res = tsd_parse_descriptor_data_stream_alignment(data, sizeof(data), &desc);
    test_assert_equal(TSD_OK, res, "TSD_OK");
    test_assert_equal(desc.tag, 0x06, "tag");
    test_assert_equal(desc.length, 0x01, "length");
    test_assert_equal(desc.type, 0x02, "type");
    test_end();
}

void test_target_background_grid(void)
{
    test_start("target_background_grid_descriptor");

    TSDDescriptorTargetBackgroundGrid desc;
    const uint8_t data[] = {
        0x07, // tag
        0x04, // length
        0xFE, 0xDC, 0xBA, 0x98, // horizontal size(14), vertical size(14), aspect ratio(4)
    };

    TSDCode res;
    res = tsd_parse_descriptor_target_background_grid(NULL, sizeof(data), &desc);
    test_assert_equal(TSD_INVALID_DATA, res, "invalid data");
    res = tsd_parse_descriptor_target_background_grid(data, 2, &desc);
    test_assert_equal(TSD_INVALID_DATA_SIZE, res, "invalid data size");
    res = tsd_parse_descriptor_target_background_grid(data, sizeof(data), NULL);
    test_assert_equal(TSD_INVALID_ARGUMENT, res, "invalid argument");

    res = tsd_parse_descriptor_target_background_grid(data, sizeof(data), &desc);
    test_assert_equal(TSD_OK, res, "TSD_OK");
    test_assert_equal(desc.tag, 0x07, "tag");
    test_assert_equal(desc.length, 0x04, "length");
    test_assert_equal(desc.horizontal_size, 0x3FB7, "horizontal size");
    test_assert_equal(desc.vertical_size, 0x0BA9, "vertical size");
    test_assert_equal(desc.aspect_ratio_info, 0x08, "aspect ratio info");
    test_end();
}

void test_video_window(void)
{
    test_start("video_window_descriptor");

    TSDDescriptorVideoWindow desc;
    const uint8_t data[] = {
        0x08, // tag
        0x04, // length
        0xAB, 0x09, 0xF3, 0x62, // horizontal offset(14), vertical offset(14), windows priority(4)
    };

    TSDCode res;
    res = tsd_parse_descriptor_video_window(NULL, sizeof(data), &desc);
    test_assert_equal(TSD_INVALID_DATA, res, "invalid data");
    res = tsd_parse_descriptor_video_window(data, 2, &desc);
    test_assert_equal(TSD_INVALID_DATA_SIZE, res, "invalid data size");
    res = tsd_parse_descriptor_video_window(data, sizeof(data), NULL);
    test_assert_equal(TSD_INVALID_ARGUMENT, res, "invalid argument");

    res = tsd_parse_descriptor_video_window(data, sizeof(data), &desc);
    test_assert_equal(TSD_OK, res, "TSD_OK");
    test_assert_equal(desc.tag, 0x08, "tag");
    test_assert_equal(desc.length, 0x04, "length");
    test_assert_equal(desc.horizontal_offset, 0x2AC2, "horizontal offset");
    test_assert_equal(desc.vertical_offset, 0x1F36, "vertical offset");
    test_assert_equal(desc.window_priority, 0x02, "window priority");
    test_end();
}

void test_conditional_access(void)
{
    test_start("conditional_access_descriptor");

    TSDDescriptorConditionalAccess desc;
    const uint8_t data[] = {
        0x09, // tag
        0x07, // length
        0x99, 0x88, // CA System ID
        0xEF, 0xD7, // reserved(3), CA_PID (13)
        0xAB, 0xCD, 0xEF, // private data bytes
    };

    TSDCode res;
    res = tsd_parse_descriptor_conditional_access(NULL, sizeof(data), &desc);
    test_assert_equal(TSD_INVALID_DATA, res, "invalid data");
    res = tsd_parse_descriptor_conditional_access(data, 2, &desc);
    test_assert_equal(TSD_INVALID_DATA_SIZE, res, "invalid data size");
    res = tsd_parse_descriptor_conditional_access(data, sizeof(data), NULL);
    test_assert_equal(TSD_INVALID_ARGUMENT, res, "invalid argument");

    res = tsd_parse_descriptor_conditional_access(data, sizeof(data), &desc);
    test_assert_equal(TSD_OK, res, "TSD_OK");
    test_assert_equal(desc.tag, 0x09, "tag");
    test_assert_equal(desc.length, 0x07, "length");
    test_assert_equal(desc.ca_system_id, 0x9988, "CA system ID");
    test_assert_equal(desc.ca_pid, 0x0FD7, "CA PID");
    test_assert_equal(desc.private_data_bytes_length, 3, "private data bytes length");
    test_assert_equal_ptr((size_t)desc.private_data_bytes, (size_t)&data[6], "private data bytes");
    test_end();
}

void test_iso_639_language(void)
{
    test_start("iso_639_language_descriptor");

    TSDDescriptorISO639Language desc;
    const uint8_t data[] = {
        0x0A, // tag
        0x08, // length
        0x02, 0x03, 0x04, // iso 639 language code
        0x01, // audio type
        0xB7, 0x2F, 0x6A, // iso 639 language code
        0x03, // audio type
    };

    TSDCode res;
    res = tsd_parse_descriptor_iso639_language(NULL, sizeof(data), &desc);
    test_assert_equal(TSD_INVALID_DATA, res, "invalid data");
    res = tsd_parse_descriptor_iso639_language(data, 2, &desc);
    test_assert_equal(TSD_INVALID_DATA_SIZE, res, "invalid data size");
    res = tsd_parse_descriptor_iso639_language(data, sizeof(data), NULL);
    test_assert_equal(TSD_INVALID_ARGUMENT, res, "invalid argument");

    res = tsd_parse_descriptor_iso639_language(data, sizeof(data), &desc);
    test_assert_equal(TSD_OK, res, "TSD_OK");
    test_assert_equal(desc.tag, 0x0A, "tag");
    test_assert_equal(desc.length, 0x08, "length");
    test_assert_equal(desc.language_length, 2, "language length");
    test_assert_equal(desc.iso_language_code[0], 0x020304, "iso 639 language 0");
    test_assert_equal(desc.audio_type[0], 0x01, "audio type 0");
    test_assert_equal(desc.iso_language_code[1], 0xB72F6A, "iso 639 language 1");
    test_assert_equal(desc.audio_type[1], 0x03, "audio type 1");
    test_end();
}

void test_scd(void)
{
    test_start("system_clock_descriptor");

    TSDDescriptorSystemClock desc;
    const uint8_t data[] = {
        0x0B, // tag
        0x02, // length
        0xD5, // external clock ref indicator (1), reserved (1), clock accuracy int (6)
        0x9F, // clock accuracy exponent (3), reserved (5)
    };

    TSDCode res;
    res = tsd_parse_descriptor_system_clock(NULL, sizeof(data), &desc);
    test_assert_equal(TSD_INVALID_DATA, res, "invalid data");
    res = tsd_parse_descriptor_system_clock(data, 3, &desc);
    test_assert_equal(TSD_INVALID_DATA_SIZE, res, "invalid data size");
    res = tsd_parse_descriptor_system_clock(data, sizeof(data), NULL);
    test_assert_equal(TSD_INVALID_ARGUMENT, res, "invalid argument");

    res = tsd_parse_descriptor_system_clock(data, sizeof(data), &desc);
    test_assert_equal(TSD_OK, res, "TSD_OK");
    test_assert_equal(desc.tag, 0x0B, "tag");
    test_assert_equal(desc.length, 0x02, "length");
    test_assert_equal(desc.external_clock_reference_indicator, 1, "clock ref indicator");
    test_assert_equal(desc.clock_accuracy_integer, 0x15, "clock accuracy integer");
    test_assert_equal(desc.clock_accuracy_exponent, 0x04, "clock accuracy exponent");

    test_end();
}

void test_multiplex_buffer_util(void)
{
    test_start("multiplex_buffer_util_descriptor");

    TSDDescriptorMultiplexBufferUtilization desc;
    const uint8_t data[] = {
        0x0C, // tag
        0x04, // length
        0xA7, 0xFE, // bound valid flag (1), ltw offset lower bound (15)
        0xB6, 0x06, // reserved (1), lts offset upper bound (15)
    };

    TSDCode res;
    res = tsd_parse_descriptor_multiplex_buffer_utilization(NULL, sizeof(data), &desc);
    test_assert_equal(TSD_INVALID_DATA, res, "invalid data");
    res = tsd_parse_descriptor_multiplex_buffer_utilization(data, 5, &desc);
    test_assert_equal(TSD_INVALID_DATA_SIZE, res, "invalid data size");
    res = tsd_parse_descriptor_multiplex_buffer_utilization(data, sizeof(data), NULL);
    test_assert_equal(TSD_INVALID_ARGUMENT, res, "invalid argument");

    res = tsd_parse_descriptor_multiplex_buffer_utilization(data, sizeof(data), &desc);
    test_assert_equal(TSD_OK, res, "TSD_OK");
    test_assert_equal(desc.tag, 0x0C, "tag");
    test_assert_equal(desc.length, 0x04, "length");
    test_assert_equal(desc.bound_valid_flag, 1, "bound valid flag");
    test_assert_equal(desc.ltw_offset_lower_bound, 0x27FE, "ltw offset lower bound");
    test_assert_equal(desc.ltw_offset_upper_bound, 0x3606, "ltw offset upper bound");

    test_end();
}

void test_copyright(void)
{
    test_start("copyright_descriptor");

    TSDDescriptorCopyright desc;
    const uint8_t data[] = {
        0x0D, // tag
        0x07, // length
        0xA8, 0xFF, 0xAA, 0x33, // copyright identifier (32)
        0x99, 0x88, 0x77, // additional copyright info (N)
    };

    TSDCode res;
    res = tsd_parse_descriptor_copyright(NULL, sizeof(data), &desc);
    test_assert_equal(TSD_INVALID_DATA, res, "invalid data");
    res = tsd_parse_descriptor_copyright(data, 5, &desc);
    test_assert_equal(TSD_INVALID_DATA_SIZE, res, "invalid data size");
    res = tsd_parse_descriptor_copyright(data, sizeof(data), NULL);
    test_assert_equal(TSD_INVALID_ARGUMENT, res, "invalid argument");

    res = tsd_parse_descriptor_copyright(data, sizeof(data), &desc);
    test_assert_equal(TSD_OK, res, "TSD_OK");
    test_assert_equal(desc.tag, 0x0D, "tag");
    test_assert_equal(desc.length, 0x07, "length");
    test_assert_equal(desc.identifier, 0xA8FFAA33, "length");
    test_assert_equal_ptr((size_t)desc.additional_copy_info, (size_t)(&data[6]), "additional copy info");
    test_assert_equal(desc.additional_copy_info_length, 0x03, "additional copy info length");

    test_end();
}

void test_max_bitrate(void)
{
    test_start("max_bitrate_descriptor");

    TSDDescriptorMaxBitrate desc;
    const uint8_t data[] = {
        0x0E, // tag
        0x03, // length
        0xE5, 0xF4, 0xAC,// reserved (2), maximum bitrate (22)
    };

    TSDCode res;
    res = tsd_parse_descriptor_max_bitrate(NULL, sizeof(data), &desc);
    test_assert_equal(TSD_INVALID_DATA, res, "invalid data");
    res = tsd_parse_descriptor_max_bitrate(data, 4, &desc);
    test_assert_equal(TSD_INVALID_DATA_SIZE, res, "invalid data size");
    res = tsd_parse_descriptor_max_bitrate(data, sizeof(data), NULL);
    test_assert_equal(TSD_INVALID_ARGUMENT, res, "invalid argument");

    res = tsd_parse_descriptor_max_bitrate(data, sizeof(data), &desc);
    test_assert_equal(TSD_OK, res, "TSD_OK");
    test_assert_equal(desc.tag, 0x0E, "tag");
    test_assert_equal(desc.length, 0x03, "length");
    test_assert_equal(desc.max_bitrate, 0x25F4AC, "max bitrate");

    test_end();
}

void test_private_data_indicator(void)
{
    test_start("priv_data_indicator_descriptor");

    TSDDescriptorPrivDataInd desc;
    const uint8_t data[] = {
        0x0F, // tag
        0x04, // length
        0xC3, 0xFE, 0x07, 0x1F// private data indicator (32)
    };

    TSDCode res;
    res = tsd_parse_descriptor_priv_data_ind(NULL, sizeof(data), &desc);
    test_assert_equal(TSD_INVALID_DATA, res, "invalid data");
    res = tsd_parse_descriptor_priv_data_ind(data, 5, &desc);
    test_assert_equal(TSD_INVALID_DATA_SIZE, res, "invalid data size");
    res = tsd_parse_descriptor_priv_data_ind(data, sizeof(data), NULL);
    test_assert_equal(TSD_INVALID_ARGUMENT, res, "invalid argument");

    res = tsd_parse_descriptor_priv_data_ind(data, sizeof(data), &desc);
    test_assert_equal(TSD_OK, res, "TSD_OK");
    test_assert_equal(desc.tag, 0x0F, "tag");
    test_assert_equal(desc.length, 0x04, "length");
    test_assert_equal(desc.private_data_indicator, 0xC3FE071F, "private data indicator");

    test_end();
}

void test_smoothing_buffer(void)
{
    test_start("smoothing_buffer_descriptor");

    TSDDescriptorSmoothingBuffer desc;
    const uint8_t data[] = {
        0x10, // tag
        0x06, // length
        0xC1, 0x65, 0xC7, // reserved (2), sb leak rate (22)
        0xD3, 0x17, 0x5A, // reserved (2), sb size (22)
    };

    TSDCode res;
    res = tsd_parse_descriptor_smoothing_buffer(NULL, sizeof(data), &desc);
    test_assert_equal(TSD_INVALID_DATA, res, "invalid data");
    res = tsd_parse_descriptor_smoothing_buffer(data, 2, &desc);
    test_assert_equal(TSD_INVALID_DATA_SIZE, res, "invalid data size");
    res = tsd_parse_descriptor_smoothing_buffer(data, sizeof(data), NULL);
    test_assert_equal(TSD_INVALID_ARGUMENT, res, "invalid argument");

    res = tsd_parse_descriptor_smoothing_buffer(data, sizeof(data), &desc);
    test_assert_equal(TSD_OK, res, "TSD_OK");
    test_assert_equal(desc.tag, 0x10, "tag");
    test_assert_equal(desc.length, 0x06, "length");
    test_assert_equal(desc.sb_leak_rate, 0x0165C7, "sb leak rate");
    test_assert_equal(desc.sb_size, 0x13175A, "sb size");

    test_end();
}

void test_std(void)
{
    test_start("std_descriptor");

    TSDDescriptorSysTargetDecoder desc;
    const uint8_t data[] = {
        0x11, // tag
        0x01, // length
        0xFE, // reserved (7), leak valid flag (1)
    };

    TSDCode res;
    res = tsd_parse_descriptor_sys_target_decoder(NULL, sizeof(data), &desc);
    test_assert_equal(TSD_INVALID_DATA, res, "invalid data");
    res = tsd_parse_descriptor_sys_target_decoder(data, 2, &desc);
    test_assert_equal(TSD_INVALID_DATA_SIZE, res, "invalid data size");
    res = tsd_parse_descriptor_sys_target_decoder(data, sizeof(data), NULL);
    test_assert_equal(TSD_INVALID_ARGUMENT, res, "invalid argument");

    res = tsd_parse_descriptor_sys_target_decoder(data, sizeof(data), &desc);
    test_assert_equal(TSD_OK, res, "TSD_OK");
    test_assert_equal(desc.tag, 0x11, "tag");
    test_assert_equal(desc.length, 0x01, "length");
    test_assert_equal(desc.leak_valid_flag, 0, "leak valid flag");

    test_end();
}

void test_ibp(void)
{
    test_start("ibp_descriptor");

    TSDDescriptorIBP desc;
    const uint8_t data[] = {
        0x12, // tag
        0x02, // length
        0x7B, 0x7F, // closed gop flag (1), identical gop flag (1), max gop length (14)
    };

    TSDCode res;
    res = tsd_parse_descriptor_ibp(NULL, sizeof(data), &desc);
    test_assert_equal(TSD_INVALID_DATA, res, "invalid data");
    res = tsd_parse_descriptor_ibp(data, 3, &desc);
    test_assert_equal(TSD_INVALID_DATA_SIZE, res, "invalid data size");
    res = tsd_parse_descriptor_ibp(data, sizeof(data), NULL);
    test_assert_equal(TSD_INVALID_ARGUMENT, res, "invalid argument");

    res = tsd_parse_descriptor_ibp(data, sizeof(data), &desc);
    test_assert_equal(TSD_OK, res, "TSD_OK");
    test_assert_equal(desc.tag, 0x12, "tag");
    test_assert_equal(desc.length, 0x02, "length");
    test_assert_equal(desc.closed_gop_flag, 0, "closed gop flag");
    test_assert_equal(desc.identical_gop_flag, 1, "identical gop flag");
    test_assert_equal(desc.max_gop_length, 0x3B7F, "identical gop flag");

    test_end();
}

void test_mpeg4_video(void)
{
    test_start("mpeg4_video_descriptor");

    TSDDescriptorMPEG4Video desc;
    const uint8_t data[] = {
        0x1B, // tag
        0x01, // length
        0x10, // mpeg4 visual profile profile and level
    };

    TSDCode res;
    res = tsd_parse_descriptor_mpeg4_video(NULL, sizeof(data), &desc);
    test_assert_equal(TSD_INVALID_DATA, res, "invalid data");
    res = tsd_parse_descriptor_mpeg4_video(data, 2, &desc);
    test_assert_equal(TSD_INVALID_DATA_SIZE, res, "invalid data size");
    res = tsd_parse_descriptor_mpeg4_video(data, sizeof(data), NULL);
    test_assert_equal(TSD_INVALID_ARGUMENT, res, "invalid argument");

    res = tsd_parse_descriptor_mpeg4_video(data, sizeof(data), &desc);
    test_assert_equal(TSD_OK, res, "TSD_OK");
    test_assert_equal(desc.tag, 0x1B, "tag");
    test_assert_equal(desc.length, 0x01, "length");
    test_assert_equal(desc.visual_profile_and_level, 0x10, "length");

    test_end();
}

void test_mpeg4_audio(void)
{
    test_start("mpeg4_audio_descriptor");

    TSDDescriptorMPEG4Audio desc;
    const uint8_t data[] = {
        0x1C, // tag
        0x01, // length
        0x11, // audio profile and level
    };

    TSDCode res;
    res = tsd_parse_descriptor_mpeg4_audio(NULL, sizeof(data), &desc);
    test_assert_equal(TSD_INVALID_DATA, res, "invalid data");
    res = tsd_parse_descriptor_mpeg4_audio(data, 2, &desc);
    test_assert_equal(TSD_INVALID_DATA_SIZE, res, "invalid data size");
    res = tsd_parse_descriptor_mpeg4_audio(data, sizeof(data), NULL);
    test_assert_equal(TSD_INVALID_ARGUMENT, res, "invalid argument");

    res = tsd_parse_descriptor_mpeg4_audio(data, sizeof(data), &desc);
    test_assert_equal(TSD_OK, res, "TSD_OK");
    test_assert_equal(desc.tag, 0x1C, "tag");
    test_assert_equal(desc.length, 0x01, "length");
    test_assert_equal(desc.audio_profile_and_level, 0x11, "audio profile and level");

    test_end();
}

void test_iod(void)
{
    test_start("iod_descriptor");

    TSDDescriptorIOD desc;
    const uint8_t data[] = {
        0x1D, // tag
        0x07, // length
        0x10, // scope of iod label (8)
        0x29, // iod label (8)
        0xFF, 0x03, // initial object descriptor, tag(8), length(8) etc.
        0x01, 0x02, 0x03
    };

    TSDCode res;
    res = tsd_parse_descriptor_iod(NULL, sizeof(data), &desc);
    test_assert_equal(TSD_INVALID_DATA, res, "invalid data");
    res = tsd_parse_descriptor_iod(data, 3, &desc);
    test_assert_equal(TSD_INVALID_DATA_SIZE, res, "invalid data size");
    res = tsd_parse_descriptor_iod(data, sizeof(data), NULL);
    test_assert_equal(TSD_INVALID_ARGUMENT, res, "invalid argument");

    res = tsd_parse_descriptor_iod(data, sizeof(data), &desc);
    test_assert_equal(TSD_OK, res, "TSD_OK");
    test_assert_equal(desc.tag, 0x1D, "tag");
    test_assert_equal(desc.length, 0x07, "length");
    test_assert_equal(desc.scope_of_iod_label, 0x10, "scope of iod label");
    test_assert_equal(desc.iod_label, 0x29, "iod label");
    test_assert_equal_ptr((size_t)desc.initial_object_descriptor, (size_t)&data[4], "initial object descriptor");
    test_assert_equal(desc.initial_object_descriptor_length, 5, "initial object descriptor length");

    test_end();
}

void test_sl(void)
{
    test_start("sl_descriptor");

    TSDDescriptorSL desc;
    const uint8_t data[] = {
        0x1E, // tag
        0x02, // length
        0xF4, 0xDE, // es id
    };

    TSDCode res;
    res = tsd_parse_descriptor_sl(NULL, sizeof(data), &desc);
    test_assert_equal(TSD_INVALID_DATA, res, "invalid data");
    res = tsd_parse_descriptor_sl(data, 3, &desc);
    test_assert_equal(TSD_INVALID_DATA_SIZE, res, "invalid data size");
    res = tsd_parse_descriptor_sl(data, sizeof(data), NULL);
    test_assert_equal(TSD_INVALID_ARGUMENT, res, "invalid argument");

    res = tsd_parse_descriptor_sl(data, sizeof(data), &desc);
    test_assert_equal(TSD_OK, res, "TSD_OK");
    test_assert_equal(desc.tag, 0x1E, "tag");
    test_assert_equal(desc.length, 0x02, "length");
    test_assert_equal(desc.es_id, 0xF4DE, "es id");

    test_end();
}

void test_fmc(void)
{
    test_start("fmc_descriptor");

    TSDDescriptorFMC desc;
    const uint8_t data[] = {
        0x1F, // tag
        0x09, // length
        0x01, 0x02, 0x03, // es id(16), flex mux channel (8)
        0xFF, 0xFE, 0xFD, // es id(16), flex mux channel (8)
        0x66, 0x55, 0x77, // es id(16), flex mux channel (8)
    };

    TSDCode res;
    res = tsd_parse_descriptor_fmc(NULL, sizeof(data), &desc);
    test_assert_equal(TSD_INVALID_DATA, res, "invalid data");
    res = tsd_parse_descriptor_fmc(data, 1, &desc);
    test_assert_equal(TSD_INVALID_DATA_SIZE, res, "invalid data size");
    res = tsd_parse_descriptor_fmc(data, sizeof(data), NULL);
    test_assert_equal(TSD_INVALID_ARGUMENT, res, "invalid argument");

    res = tsd_parse_descriptor_fmc(data, sizeof(data), &desc);
    test_assert_equal(TSD_OK, res, "TSD_OK");
    test_assert_equal(desc.tag, 0x1F, "tag");
    test_assert_equal(desc.length, 0x09, "length");
    test_assert_equal(desc.fmc_length, 3, "fmc length");
    test_assert_equal(desc.es_id[0], 0x0102, "es id 0");
    test_assert_equal(desc.flex_mux_channel[0], 0x03, "flex mux channel 0");
    test_assert_equal(desc.es_id[1], 0xFFFE, "es id 1");
    test_assert_equal(desc.flex_mux_channel[1], 0xFD, "flex mux channel 1");
    test_assert_equal(desc.es_id[2], 0x6655, "es id 2");
    test_assert_equal(desc.flex_mux_channel[2], 0x77, "flex mux channel 2");

    test_end();
}

void test_external_es_id(void)
{
    test_start("external_es_id_descriptor");

    TSDDescriptorExternalESID desc;
    const uint8_t data[] = {
        0x20, // tag
        0x02, // length
        0xF3, 0x7D, // external es id
    };

    TSDCode res;
    res = tsd_parse_descriptor_external_es_id(NULL, sizeof(data), &desc);
    test_assert_equal(TSD_INVALID_DATA, res, "invalid data");
    res = tsd_parse_descriptor_external_es_id(data, 2, &desc);
    test_assert_equal(TSD_INVALID_DATA_SIZE, res, "invalid data size");
    res = tsd_parse_descriptor_external_es_id(data, sizeof(data), NULL);
    test_assert_equal(TSD_INVALID_ARGUMENT, res, "invalid argument");

    res = tsd_parse_descriptor_external_es_id(data, sizeof(data), &desc);
    test_assert_equal(TSD_OK, res, "TSD_OK");
    test_assert_equal(desc.tag, 0x20, "tag");
    test_assert_equal(desc.length, 0x02, "length");
    test_assert_equal(desc.es_id, 0xF37D, "es id");

    test_end();
}

void test_mux_code(void)
{
    test_start("mux_code_descriptor");

    TSDDescriptorMuxCode desc;
    const uint8_t data[] = {
        0x21, // tag
        0x0A, // length
        0x05, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // mux code table entry, length(8)...
        0x03, 0xFF, 0xFF, 0xFF, // mux code table entry, length(8)...
    };

    TSDCode res;
    res = tsd_parse_descriptor_mux_code(NULL, sizeof(data), &desc);
    test_assert_equal(TSD_INVALID_DATA, res, "invalid data");
    res = tsd_parse_descriptor_mux_code(data, 1, &desc);
    test_assert_equal(TSD_INVALID_DATA_SIZE, res, "invalid data size");
    res = tsd_parse_descriptor_mux_code(data, sizeof(data), NULL);
    test_assert_equal(TSD_INVALID_ARGUMENT, res, "invalid argument");

    res = tsd_parse_descriptor_mux_code(data, sizeof(data), &desc);
    test_assert_equal(TSD_OK, res, "TSD_OK");
    test_assert_equal(desc.tag, 0x21, "tag");
    test_assert_equal(desc.length, 0x0A, "length");
    test_assert_equal_ptr((size_t)desc.mux_code_table_entries, (size_t)&data[2], "mux code table entries");
    test_assert_equal(desc.mux_code_table_entries_length, 10, "mux code table entries length");

    test_end();
}

void test_fmx_buffer_size(void)
{
    test_start("fmx_buffer_size_descriptor");

    TSDDescriptorFMXBufferSize desc;
    const uint8_t data[] = {
        0x22, // tag
        0x0F, // length
        0x01, 0x04, 0xFF, 0xFF, 0xFF, 0xFF,// default mux buffer descriptor, index(8), length(8), ...
        0x02, 0x02, 0xFF, 0xFF, // Flex Mux Buffer Descriptor, index(8), length(8), ...
        0x03, 0x03, 0xFF, 0xFF, 0xFF, // Flex Mux Buffer Descriptor, index(8), length(8), ...
    };

    TSDCode res;
    res = tsd_parse_descriptor_fmx_buffer_size(NULL, sizeof(data), &desc);
    test_assert_equal(TSD_INVALID_DATA, res, "invalid data");
    res = tsd_parse_descriptor_fmx_buffer_size(data, 3, &desc);
    test_assert_equal(TSD_INVALID_DATA_SIZE, res, "invalid data size");
    res = tsd_parse_descriptor_fmx_buffer_size(data, sizeof(data), NULL);
    test_assert_equal(TSD_INVALID_ARGUMENT, res, "invalid argument");

    res = tsd_parse_descriptor_fmx_buffer_size(data, sizeof(data), &desc);
    test_assert_equal(TSD_OK, res, "TSD_OK");
    test_assert_equal(desc.tag, 0x22, "tag");
    test_assert_equal(desc.length, 0x0F, "length");
    test_assert_equal_ptr((size_t)desc.default_flex_mux_buffer_descriptor, (size_t)&data[2], "default flex mux buffer descriptor");
    test_assert_equal(desc.default_flex_mux_buffer_descriptor_length, 0x06, "default flex mux buffer descriptor length");
    test_assert_equal_ptr((size_t)desc.flex_mux_buffer_descriptors, (size_t)&data[8], "flex mux buffer descriptors");
    test_assert_equal(desc.flex_mux_buffer_descriptors_length, 0x09, "flex mux buffer descriptors length");

    test_end();
}

void test_multiplex_buffer(void)
{
    test_start("multiplex_buffer_descriptor");

    TSDDescriptorMultiplexBuffer desc;
    const uint8_t data[] = {
        0x23, // tag
        0x06, // length
        0xFF, 0xEE, 0xDD, // mb buffer size (24)
        0x11, 0x22, 0x33, // tb leak rate (24)
    };

    TSDCode res;
    res = tsd_parse_descriptor_multiplex_buffer(NULL, sizeof(data), &desc);
    test_assert_equal(TSD_INVALID_DATA, res, "invalid data");
    res = tsd_parse_descriptor_multiplex_buffer(data, 7, &desc);
    test_assert_equal(TSD_INVALID_DATA_SIZE, res, "invalid data size");
    res = tsd_parse_descriptor_multiplex_buffer(data, sizeof(data), NULL);
    test_assert_equal(TSD_INVALID_ARGUMENT, res, "invalid argument");

    res = tsd_parse_descriptor_multiplex_buffer(data, sizeof(data), &desc);
    test_assert_equal(TSD_OK, res, "TSD_OK");
    test_assert_equal(desc.tag, 0x23, "tag");
    test_assert_equal(desc.length, 0x06, "length");
    test_assert_equal(desc.mb_buffer_size, 0xFFEEDD, "mb buffer size");
    test_assert_equal(desc.tb_leak_rate, 0x112233, "tb leak rate");

    test_end();
}
