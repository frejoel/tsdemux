/*
 * MIT License
 *
 * Copyright (c) 2017 Joel Freeman
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef TS_DEMUX_H
#define TS_DEMUX_H

#include <stdlib.h>
#include <stdint.h>

#define TSD_SYNC_BYTE                         (0x47)
#define TSD_MESSAGE_LEN                       (128)
#define TSD_TSPACKET_SIZE                     (188)

/**
 * @file
 * @ingroup libts
 * Libts external API header
 */

/**
 * Memory Allocator.
 */
typedef void * (*tsd_malloc) (size_t size);

/**
 * Memory deallocator.
 */
typedef void (*tsd_free) (void *mem);

/**
 * Return codes.
 */
typedef enum TSCode {
    TSD_OK                                    = 0x0000,
    TSD_INVALID_SYNC_BYTE                     = 0x0001,
    TSD_INVALID_CONTEXT                       = 0x0002,
    TSD_INVALID_DATA                          = 0x0003,
    TSD_INVALID_DATA_SIZE                     = 0x0004,
    TSD_INVALID_ARGUMENT                      = 0x0005,
} TSCode;

typedef enum TSPacketFlags {
    TSPF_TRANSPORT_ERROR_INDICATOR            = 0x01,
    TSPF_PAYLOAD_UNIT_START_INDICATOR         = 0x02,
    TSPF_TRANSPORT_PRIORITY                   = 0x03,
} TSPacketFlags;

/**
 * Adaptation Field Control.
 */
typedef enum AdaptionFieldControl {
    AFC_RESERVED                              = 0x00,
    AFC_NO_FIELD_PRESENT                      = 0x01,
    AFC_ADAPTATION_FIELD_ONLY                 = 0x02,
    AFC_ADAPTATION_FIELD_AND_PAYLOAD          = 0x03,
} AdaptionFieldControl;

/**
 * Transport Scrambling Control.
 */
typedef enum ScramblingControl {
    /** No scrambling of packet payload **/
    SC_NO_SCRAMBLING                          = 0x00,
    /** Reserved **/
    SC_RESERVED                               = 0x01,
    /** Transport Packet scrambled with Even key. **/
    SC_EVEN_KEY                               = 0x02,
    /** Transport Packet scrambled with Odd key. **/
    SC_ODD_KEY                                = 0x03,
} ScramblingControl;

/**
 * PID Value Allocations
 */
typedef enum PIDValueAllocation {
    /** Program Association Table **/
    PID_PAT                                   = 0x0000,
    /** Conditional Access Table **/
    PID_CAT                                   = 0x0001,
    /** Transport Stream Description Table **/
    PID_TSDT                                  = 0x0002,
    /** MPEG-2 Systems Reserved **/
    PID_RESERVED_MPEG2                        = 0x000F,
    /** DVB Service Infomation **/
    PID_RESERVED_DVB_SI                       = 0x001F,
    /** ARIB Service Information **/
    PID_RESERVED_ARIB_SI                      = 0x002F,
    /** Set aside for non ATSC Use **/
    PID_RESERVED_NON_ATSC                     = 0x004F,
    /** Available for general purpose **/
    PID_GENERAL_PURPOSE                       = 0x1FEF,
    /** Reserved for possible future use by ATSC and/or SCTE **/
    PID_RESERVED_FUTURE                       = 0x1FFA,
    /** ATSC PSIP tables **/
    PID_ATSC_PSIP_SI                          = 0x1FFB,
    /** Used by now-obsolete ATSC A/55 and A/56 standards **/
    PID_RESERVED_ATSC                         = 0x1FFD,
    /** OpenCable Data-Over-Cable Service Interface **/
    PID_DOCSIS                                = 0x1FFE,
    /** Identifies Null Packets **/
    PID_NULL_PACKETS                          = 0x1FFF,
} PIDValueAllocation;

/**
 * Adaptation Field Flags.
 */
typedef enum AdaptationFieldFlags {
    AF_DISCONTINUITY_INDICATOR                = 0x01,
    AF_RANDOM_ACCESS_INDICATOR                = 0x02,
    AF_ELEMENTARY_STREAM_PRIORIY_INDICATOR    = 0x03,
    AF_PCR_FLAG                               = 0x04,
    AF_OPCR_FLAG                              = 0x05,
    AF_SPLICING_POINT_FLAG                    = 0x06,
    AF_TRANSPORT_PRIVATE_DATA_FLAG            = 0x07,
    AF_ADAPTATION_FIELD_EXTENSION_FLAG        = 0x08,
} AdaptationFieldFlags;

/**
 * Adaptation Field Extensions Flags.
 */
typedef enum AdaptationFieldExtensionFlags {
    AFEF_LTW_FLAG                             = 0x01,
    AFEF_PIECEWISE_RATE_FLAG                  = 0x02,
    AFEF_SEAMLESS_SPLCE_FLAG                  = 0x03,
} AdaptationFieldExtensionFlags;

/**
 * PES Scrambling Control.
 */
typedef enum PESScramblingControl {
    PSC_NOT_SCRAMBLED                         = 0x00,
    PSC_USER_DEFINED_1                        = 0x01,
    PSC_USER_DEFINED_2                        = 0x02,
    PSC_USER_DEFINED_3                        = 0x03,
} PESScramblingControl;

/**
 * PES Packaet Flags.
 */
typedef enum PESPacketFlags {
    PPF_PES_PRIORITY                          = 0x01,
    PPF_DATA_ALIGNMENT_INDICATOR              = 0x02,
    PPF_COPYRIGHT                             = 0x03,
    PPF_ORIGINAL_OR_COPY                      = 0x04,
    PPF_PTS_FLAG                              = 0x05,
    PPF_DTS_FLAG                              = 0x06,
    PPF_ESCR_FLAG                             = 0x07,
    PPF_ES_RATE_FLAG                          = 0x08,
    PPF_DSM_TRICK_MODE_FLAG                   = 0x09,
    PPF_ADDITIONAL_COPY_INFO_FLAG             = 0x0A,
    PPF_PES_CRC_FLAG                          = 0x0B,
    PPF_PES_EXTENSION_FLAG                    = 0x0C,
} PESPacketFlags;

/**
 * Trick Mode Control.
 */
typedef enum TrickModeControl {
    TMC_FAST_FORWARD                          = 0x0000,
    TMC_SLOW_MOTION                           = 0x0001,
    TMC_FREEZE_FRAME                          = 0x0010,
    TMC_FAST_REVERSE                          = 0x0011,
    TMC_SLOW_REVERSE                          = 0x0100,
} TrickModeControl;

/**
 * PES Extension Flags.
 */
typedef enum PESExtensionFlags {
    PEF_PES_PRIVATE_DATA_FLAG                 = 0x01,
    PEF_PACK_HEADER_FIELD_FLAG                = 0x02,
    PEF_PROGRAM_PACKET_SEQUENCE_COUNTER_FLAG  = 0x03,
    PEF_PSTD_BUFFER_FLAG                      = 0x04,
    PEF_PES_EXTENSION_FLAG_2                  = 0x08,
} PESExtensionFlags;

/**
 * System Header Flags.
 */
typedef enum SystemHeaderFlags {
    SHF_FIXED_FLAG                            = 0x01,
    SHF_CSPS_FLAG                             = 0x02,
    SHF_SYSTEM_AUDIO_LOCK_FLAG                = 0x03,
    SHF_SYSTEM_VIDEO_LOCK_FLAG                = 0x04,
    SHF_PACKET_RATE_RESTICTION_FLAG           = 0x10,
} SystemHeaderFlags;

/**
 * TS Demux Context.
 * The TSDEmuxContext is used to separate multiple demux tasks.
 */
typedef struct TSDemuxContext {
    tsd_malloc malloc;
    tsd_free free;
    char last_error_msg[TSD_MESSAGE_LEN];
} TSDemuxContext;

/**
 * Adaptation Field Extension.
 */
typedef struct AdapationFieldExtension {
    uint8_t length;
    int flags;
    // ltw_flag == '1'
    int ltw_valid_flag;
    uint32_t ltw_valid_offset;
    // piecewise_rate_flag == '1'
    uint32_t piecewise_rate;
    // seamless_splice_flag == '1'
    uint8_t splice_type;
    uint64_t dts_next_au;
} AdapationFieldExtension;

/**
 * Adaptation Field.
 */
typedef struct AdaptationField {
    uint8_t adaptation_field_length;
    uint8_t flags;
    // PCR == '1'
    uint64_t program_clock_reference_base;
    int program_clock_reference_extension;
    // OPCR == '1'
    uint64_t original_program_clock_reference_base;
    int original_program_clock_reference_extension;
    // splicing_point_fag == '1'
    uint8_t splice_countdown;
    // transport provate data flag == '1'
    uint8_t transport_private_date_length;
    void *private_data_byte;
    // adaptation_field_extension_flag == '1'
    AdapationFieldExtension adaptation_field_extension;
} AdaptationField;

/**
 * Transport Stream Packet Header.
 */
typedef struct TSPacket {
    uint8_t sync_byte;
    int flags;
    uint16_t pid;
    ScramblingControl transport_scrambling_control;
    AdaptionFieldControl adaptation_field_control;
    uint8_t continuity_counter;
    AdaptationField adaptation_field;
    void *data_bytes;
} TSPacket;

/**
 * DSM Trick Mode.
 */
typedef struct DSMTrickMode {
    TrickModeControl trick_mode_control;
    uint8_t field_id;
    int intra_slice_refresh;
    uint8_t frequency_truncation;
    uint8_t rep_cntrl;
} DSMTrickMode;

/**
 * Program Stream System Header.
 */
typedef struct SystemHeader {
    uint32_t system_header_start_code;
    uint16_t header_length;
    uint32_t rate_bound;
    uint8_t audio_bound;
    int system_header_flags;
    uint8_t video_bound;
    uint8_t stream_id;
    int pstd_buffer_bound_scale;
    uint8_t pstd_buffer_size_bound;
} SystemHeader;

/**
 * Program Stream Pack Header
 */
typedef struct PackHeader {
    uint32_t pack_start_code;
    uint64_t system_clock_reference_base;
    uint16_t system_clock_reference_extension;
    uint32_t program_mux_rate;
    uint32_t system_header_start_code;
    SystemHeader system_header;
} PackHeader;

typedef struct PESExtension {
    int flags;
    // pes_private_data_flag == '1'
    uint8_t pes_private_data[16]; // 128 bit
    // pack_header_field_flag == '1'
    uint8_t pack_header_length;
    PackHeader pack_header;
    // program_packet_sequence_counter_flag == '1'
    uint8_t program_packet_sequence_counter;
    int mpeg1_mpeg2_identifier;
    uint8_t original_stuff_length;
    // P-STD_buffer_flag == '1'
    int pstd_buffer_scale;
    uint16_t pstd_buffer_size;
    // pes_extension_flag_2 == '1'
    uint8_t pes_extension_field_length;
} PESExtension;

/**
 * PES Packet.
 */
typedef struct PESPacket {
    uint32_t packet_start_code_prefix;
    uint8_t stream_id;
    uint16_t pes_packet_length;
    PESScramblingControl pes_scrambling_control;
    int flags;
    uint8_t pes_header_data_length;
    uint64_t pts;
    uint64_t dts;
    uint64_t escr;
    uint16_t escr_extension;
    uint32_t es_rate;
    DSMTrickMode dsm_trick_mode;
    uint8_t additional_copy_info;
    uint16_t previous_pes_packet_crc;
    void *pes_packet_data_byte;
} PESPacket;

TSCode parse_packet_header(TSDemuxContext *ctx,
                           const void *data,
                           size_t size,
                           TSPacket *hdr);

TSCode parse_adaptation_field(TSDemuxContext *ctx,
                              const TSPacket *data,
                              AdaptationField *adap);

#endif // TS_DEMUX_H
