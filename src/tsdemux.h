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

// Software version, format MAJOR.MINOR.PATCH
#define TSD_VERSION                             "0.1.1"

#define TSD_SYNC_BYTE                           (0x47)
#define TSD_MESSAGE_LEN                         (128)
#define TSD_TSPACKET_SIZE                       (188)
#define TSD_MEM_PAGE_SIZE                       (1024)
#define TSD_MAX_PID_REGS                        (16)

// C++ support
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @file
 * @ingroup tsdemux
 * Libtsdemux external API header
 */

// forward declarations
typedef struct TSDemuxContext TSDemuxContext;
typedef struct TSDTable TSDTable;
typedef struct TSDTableSection TSDTableSection;

/**
 * Event Id.
 * The Id of all events that may occur during demux. 
 * (Moved here because forward declaration not allowed on enum!)
 */
typedef enum TSDEventId {
    TSD_EVENT_PAT                            = 0x0001,
    TSD_EVENT_PMT                            = 0x0002,
    TSD_EVENT_CAT                            = 0x0004,
    TSD_EVENT_TSDT                           = 0x0008,
    /// Unsupported TSDTable
    TSD_EVENT_TABLE                          = 0x0010,
    // User Registered PES data
    TSD_EVENT_PES                            = 0x0020,
    // User Registered Adaptionn Field Private Data
    TSD_EVENT_ADAP_FIELD_PRV_DATA            = 0x0040,
} TSDEventId;

typedef enum TSDEventId TSDEventId;

/**
 * Memory Allocator.
 * Allocate memory block.
 * See malloc C90 (C++98) definition.
 */
typedef void * (*tsd_malloc) (size_t size);

/**
 * Memory Allocator.
 * Allocate and zero initialize array.
 * See calloc C90 (C++98) definition.
 */
typedef void * (*tsd_calloc) (size_t num, size_t size);

/**
 * Memory Allocator.
 * Changes the size of the memory block pointed at by ptr.
 * See realloc C90 (C++98) definition.
 */
typedef void * (*tsd_realloc) (void *ptr, size_t size);

/**
 * Memory deallocator.
 */
typedef void (*tsd_free) (void *mem);

/**
 * Event Callback.
 */
typedef void (*tsd_on_event) (TSDemuxContext *ctx,
                              uint16_t pid,
                              TSDEventId id,
                              void *data);

/**
 * Return codes.
 */
typedef enum TSDCode {
    TSD_OK                                    = 0x0000,
    TSD_INVALID_SYNC_BYTE                     = 0x0001,
    TSD_INVALID_CONTEXT                       = 0x0002,
    TSD_INVALID_DATA                          = 0x0003,
    TSD_INVALID_DATA_SIZE                     = 0x0004,
    TSD_INVALID_ARGUMENT                      = 0x0005,
    TSD_INVALID_START_CODE_PREFIX             = 0x0006,
    TSD_OUT_OF_MEMORY                         = 0x0007,
    TSD_INCOMPLETE_TABLE                      = 0x0008,
    TSD_NOT_A_TABLE_PACKET                    = 0x0009,
    TSD_PARSE_ERROR                           = 0x000A,
    TSD_PID_ALREADY_REGISTERED                = 0x000B,
    TSD_TSD_MAX_PID_REGS_REACHED              = 0x000C,
    TSD_PID_NOT_FOUND                         = 0x000D,
    TSD_INVALID_POINTER_FIELD                 = 0x000E,
} TSDCode;

/**
 * Transport Stream Packet Flags.
 */
typedef enum TSDPacketFlags {
    TSD_PF_TRAN_ERR_INDICATOR                 = 0x04,
    TSD_PF_PAYLOAD_UNIT_START_IND             = 0x02,
    TSD_PF_TRAN_PRIORITY                      = 0x01,
} TSDPacketFlags;

/**
 * Adaptation Field Control.
 */
typedef enum TSDAdaptionFieldControl {
    TSD_AFC_RESERVED                          = 0x00,
    TSD_AFC_NO_FIELD_PRESENT                  = 0x01,
    TSD_AFC_ADAP_FIELD_ONLY                   = 0x02,
    TSD_AFC_ADAP_FIELD_AND_PAYLOAD            = 0x03,
} TSDAdaptionFieldControl;

/**
 * Transport Scrambling Control.
 */
typedef enum TSDScramblingControl {
    /** No scrambling of packet payload **/
    TSD_SC_NO_SCRAMBLING                          = 0x00,
    /** Reserved **/
    TSD_SC_RESERVED                               = 0x01,
    /** Transport Packet scrambled with Even key. **/
    TSD_SC_EVEN_KEY                               = 0x02,
    /** Transport Packet scrambled with Odd key. **/
    TSD_SC_ODD_KEY                                = 0x03,
} TSDScramblingControl;

/**
 * PID Value Allocations
 */
typedef enum TSDPIDValue {
    /** Program Association TSDTable **/
    TSD_PID_PAT                                   = 0x0000,
    /** Conditional Access TSDTable **/
    TSD_PID_CAT                                   = 0x0001,
    /** Transport Stream Description TSDTable **/
    TSD_PID_TSDT                                  = 0x0002,
    /** MPEG-2 Systems Reserved **/
    TSD_PID_RESERVED_MPEG2                        = 0x000F,
    /** SDT Service Information (of DVB) **/
    TSD_PID_SDT                                   = 0x0011,
    /** DVB Service Information **/
    TSD_PID_RESERVED_DVB_SI                       = 0x001F,
    /** PIDs from this PID can be assigned to Program Map Tables, Elementary Streams
      * or other data tables **/
    TSD_PID_DATA_TABLES_START                     = 0x0020,
    /** ARIB Service Information **/
    TSD_PID_RESERVED_ARIB_SI                      = 0x002F,
    /** Set aside for non ATSC Use **/
    TSD_PID_RESERVED_NON_ATSC                     = 0x004F,
    /** Available for general purpose **/
    TSD_PID_GENERAL_PURPOSE                       = 0x1FEF,
    /** Reserved for possible future use by ATSC and/or SCTE **/
    TSD_PID_RESERVED_FUTURE                       = 0x1FFA,
    /** ATSC PSIP tables **/
    TSD_PID_ATSC_PSIP_SI                          = 0x1FFB,
    /** Used by now-obsolete ATSC A/55 and A/56 standards **/
    TSD_PID_RESERVED_ATSC                         = 0x1FFD,
    /** OpenCable Data-Over-Cable Service Interface **/
    TSD_PID_DOCSIS                                = 0x1FFE,
    /** Identifies Null Packets **/
    TSD_PID_NULL_PACKETS                          = 0x1FFF,
} TSDPIDValue;

/**
 * Adaptation Field Flags.
 */
typedef enum TSDAdaptationFieldFlags {
    TSD_AF_DISCON_IND                             = 0x80,
    TSD_AF_RANDOM_ACCESS_IND                      = 0x40,
    TSD_AF_ELEM_STREAM_PRIORIY_IND                = 0x20,
    TSD_AF_PCR_FLAG                               = 0x10,
    TSD_AF_OPCR_FLAG                              = 0x08,
    TSD_AF_SPLICING_POINT_FLAG                    = 0x04,
    TSD_AF_TRAN_PRIVATE_DATA_FLAG                 = 0x02,
    TSD_AF_ADAP_FIELD_EXT_FLAG                    = 0x01,
} TSDAdaptationFieldFlags;

/**
 * Adaptation Field Extensions Flags.
 */
typedef enum TSDAdaptationFieldExtensionFlags {
    TSD_AFEF_LTW_FLAG                             = 0x04,
    TSD_AFEF_PIECEWISE_RATE_FLAG                  = 0x02,
    TSD_AFEF_SEAMLESS_SPLCE_FLAG                  = 0x01,
} TSDAdaptationFieldExtensionFlags;

/**
 * PES Stream Id.
 */
typedef enum TSDPESStreamId {
    TSD_PSID_PROGRAM_STREAM_MAP                   = 0xBC,
    TSD_PSID_PRIV_STREAM_1                        = 0xBD,
    TSD_PSID_PADDING_STREAM                       = 0xBE,
    TSD_PSID_PRIV_STREAM_2                        = 0xBF,
    TSD_PSID_AUDIO                                = 0xC0,
    TSD_PSID_AUDIO_MASK                           = 0xE0,
    TSD_PSID_VIDEO                                = 0xE0,
    TSD_PSID_VIDEO_MASK                           = 0xF0,
    TSD_PSID_ECM                                  = 0xF0,
    TSD_PSID_EMM                                  = 0xF1,
    TSD_PSID_DSMCC                                = 0xF2,
    TSD_PSID_13522                                = 0xF3,
    TSD_PSID_H2221_TYPE_A                         = 0xF4,
    TSD_PSID_H2221_TYPE_B                         = 0xF5,
    TSD_PSID_H2221_TYPE_C                         = 0xF6,
    TSD_PSID_H2221_TYPE_D                         = 0xF7,
    TSD_PSID_H2221_TYPE_E                         = 0xF8,
    TSD_PSID_ANCILLARY                            = 0xF9,
    TSD_PSID_SL_PACKETIZED                        = 0xFA,
    TSD_PSID_FLEX_MUX                             = 0xFB,
    TSD_PSID_RESERVED                             = 0xFC,
    TSD_PSID_STREAM_DIRECTORY                     = 0xFF,
} TSDPESStreamId;

/**
 * PES Scrambling Control.
 */
typedef enum TSDPESScramblingControl {
    TSD_PSCNOT_SCRAMBLED                         = 0x00,
    TSD_PSCUSER_DEFINED_1                        = 0x01,
    TSD_PSCUSER_DEFINED_2                        = 0x02,
    TSD_PSCUSER_DEFINED_3                        = 0x03,
} TSDPESScramblingControl;

/**
 * PES Packaet Flags.
 */
typedef enum TSDPESPacketFlags {
    TSD_PPF_PES_PRIORITY                          = 0x0800,
    TSD_PPF_DATA_ALIGNMENT_INDICATOR              = 0x0400,
    TSD_PPF_COPYRIGHT                             = 0x0200,
    TSD_PPF_ORIGINAL_OR_COPY                      = 0x0100,
    TSD_PPF_PTS_FLAG                              = 0x0080,
    TSD_PPF_DTS_FLAG                              = 0x0040,
    TSD_PPF_ESCR_FLAG                             = 0x0020,
    TSD_PPF_ES_RATE_FLAG                          = 0x0010,
    TSD_PPF_DSM_TRICK_MODE_FLAG                   = 0x0008,
    TSD_PPF_ADDITIONAL_COPY_INFO_FLAG             = 0x0004,
    TSD_PPF_PES_CRC_FLAG                          = 0x0002,
    TSD_PPF_PES_EXTENSION_FLAG                    = 0x0001,
} TSDPESPacketFlags;

/**
 * Trick Mode Control.
 */
typedef enum TSDTrickModeControl {
    TSD_TMC_FAST_FORWARD                          = 0x00,
    TSD_TMC_SLOW_MOTION                           = 0x01,
    TSD_TMC_FREEZE_FRAME                          = 0x02,
    TSD_TMC_FAST_REVERSE                          = 0x03,
    TSD_TMC_SLOW_REVERSE                          = 0x04,
} TSDTrickModeControl;

/**
 * PES Extension Flags.
 */
typedef enum TSDPESExtensionFlags {
    TSD_PEF_PES_PRIVATE_DATA_FLAG                 = 0x80,
    TSD_PEF_PACK_HEADER_FIELD_FLAG                = 0x40,
    TSD_PEF_PROGRAM_PACKET_SEQUENCE_COUNTER_FLAG  = 0x20,
    TSD_PEF_PSTD_BUFFER_FLAG                      = 0x10,
    TSD_PEF_PES_EXTENSION_FLAG_2                  = 0x01,
} TSDPESExtensionFlags;

/**
 * System Header Flags.
 */
typedef enum TSDSystemHeaderFlags {
    TSD_SHF_FIXED_FLAG                            = 0x02000000,
    TSD_SHF_CSPS_FLAG                             = 0x01000000,
    TSD_SHF_SYSTEM_AUDIO_LOCK_FLAG                = 0x00800000,
    TSD_SHF_SYSTEM_VIDEO_LOCK_FLAG                = 0x00400000,
    TSD_SHF_PACKET_RATE_RESTICTION_FLAG           = 0x00008000,
} TSDSystemHeaderFlags;

/**
 * TSDTable Flags.
 */
typedef enum TSDTableFlags {
    TSD_TBL_PRIVATE_INDICATOR                     = 0x01,
    TSD_TBL_SECTION_SYNTAX_INDICATOR              = 0x02,
    TSD_TBL_CURRENT_NEXT_INDICATOR                = 0x04,
} TSDTableFlags;

/**
 * PMT Program Element Stream Type
 */
typedef enum TSDPMTStreamType {
    TSD_PMT_STREAM_TYPE_VIDEO                        = 0x01,
    TSD_PMT_STREAM_TYPE_VIDEO_H262                   = 0x02,
    TSD_PMT_STREAM_TYPE_AUDIO_11172                  = 0x03,
    TSD_PMT_STREAM_TYPE_AUDIO_13818_3                = 0x04,
    TSD_PMT_STREAM_TYPE_PRV_SECTIONS                 = 0x05,
    TSD_PMT_STREAM_TYPE_PES_PRV                      = 0x06,
    TSD_PMT_STREAM_TYPE_MHEG                         = 0x07,
    TSD_PMT_STREAM_TYPE_H222_0_DSM_CC                = 0x08,
    TSD_PMT_STREAM_TYPE_H222_1                       = 0x09,
    TSD_PMT_STREAM_TYPE_A                            = 0x0A,
    TSD_PMT_STREAM_TYPE_B                            = 0x0B,
    TSD_PMT_STREAM_TYPE_C                            = 0x0C,
    TSD_PMT_STREAM_TYPE_D                            = 0x0D,
    TSD_PMT_STREAM_TYPE_H222_0_AUX                   = 0x0E,
    TSD_PMT_STREAM_TYPE_AUDIO_AAC                    = 0x0F,
    TSD_PMT_STREAM_TYPE_VISUAL                       = 0x10,
    TSD_PMT_STREAM_TYPE_AUDIO_LATM                   = 0x11,
    TSD_PMT_STREAM_TYPE_SL_PES                       = 0x12,
    TSD_PMT_STREAM_TYPE_SL_SECTIONS                  = 0x13,
    TSD_PMT_STREAM_TYPE_SYNC_DOWNLOAD                = 0x14,
    TSD_PMT_STREAM_TYPE_PES_METADATA                 = 0x15,
    TSD_PMT_STREAM_TYPE_METDATA_SECTIONS             = 0x16,
    TSD_PMT_STREAM_TYPE_METADATA_DATA_CAROUSEL       = 0x17,
    TSD_PMT_STREAM_TYPE_METADATA_OBJ_CAROUSEL        = 0x18,
    cTSD_PMT_STREAM_TYPE_METADATA_SYNC_DOWNLOAD      = 0x19,
    TSD_PMT_STREAM_TYPE_IPMP                         = 0x1A,
    TSD_PMT_STREAM_TYPE_VIDEO_AVC                    = 0X1B,
    TSD_PMT_STREAM_TYPE_VIDEO_H222_0                 = 0x1C,
    TSD_PMT_STREAM_TYPE_DCII_VIDEO                   = 0x80,
    TSD_PMT_STREAM_TYPE_AUDIO_A53                    = 0x81,
    TSD_PMT_STREAM_TYPE_SCTE_STD_SUBTITLE            = 0x82,
    TSD_PMT_STREAM_TYPE_SCTE_ISOCH_DATA              = 0x83,
    TSD_PMT_STREAM_TYPE_ATSC_PROG_ID                 = 0x85,
    TSD_PMT_STREAM_TYPE_SCTE_25                      = 0x86,
    TSD_PMT_STREAM_TYPE_AUDIO_EAC3                   = 0x87,
    TSD_PMT_STREAM_TYPE_AUDIO_DTS_HD                 = 0x88,
    TSD_PMT_STREAM_TYPE_DVB_MPE_FEC                  = 0x90,
    TSD_PMT_STREAM_TYPE_ULE                          = 0x91,
    TSD_PMT_STREAM_TYPE_VEI                          = 0x92,
    TSD_PMT_STREAM_TYPE_ATSC_DATA_SERVICE_TABLE      = 0x95,
    TSD_PMT_STREAM_TYPE_SCTE_IP_DATA                 = 0xA0,
    TSD_PMT_STREAM_TYPE_DCII_TEXT                    = 0xC0,
    TSD_PMT_STREAM_TYPE_ATSC_SYNC_DATA               = 0xC2,
    TSD_PMT_STREAM_TYPE_SCTE_AYSNC_DATA              = 0xC3,
    TSD_PMT_STREAM_TYPE_ATSC_USER_PRIV_PROG_ELEMENTS = 0xC4,
    TSD_PMT_STREAM_TYPE_VC1                          = 0xEA,
    TSD_PMT_STREAM_TYPE_ATSC_USER_PRIV               = 0xEB,
} TSDPMTStreamType;


/**
 * Registration Type.
 * Flags identifying what type of data the user wants to register.
 * @see tsd_register_pid
 */
typedef enum TSDRegType {
    TSD_REG_PES                     = 0x01,
    TSD_REG_ADAPTATION_FIELD        = 0x02,
} TSDRegType;

/**
 * Video Stream Descriptor Flags.
 */
typedef enum TSDDescriptorFlagsVideoStream {
    TSD_DFVS_MULTI_FRAME_RATE       = 0x80,
    TSD_DFVS_MPEG_1_ONLY            = 0x04,
    TSD_DFVS_CONSTRAINED_PARAM      = 0x02,
    TSD_DFVS_STILL_PIC              = 0x01,
    TSD_DFVS_FRAME_RATE_EXT         = 0x20,
} TSDDescriptorFlagsVideoStream;

/**
 * AUdio Stream Descriptor Flags.
 */
typedef enum TSDDescriptorFlagsAudioStream {
    TSD_DFAS_FREE_FORMAT            = 0x80,
    TSD_DFAS_ID                     = 0x40,
    TSD_DFAS_VAR_RATE_AUDIO_IND     = 0x08,
} TSDDescriptorFlagsAudioStream;

/**
 * Data Context.
 * Used to persist the session when streaming TS packets through the demux in
 * multiple calls.
 */
typedef struct TSDDataContext {
    uint8_t *buffer;
    uint8_t *write;
    uint8_t *end;
    size_t size;
    uint32_t id;
} TSDDataContext;

/**
 * Adaptation Field Extension.
 */
typedef struct TSDAdaptationFieldExtension {
    uint8_t length;
    int flags;
    // ltw_flag == '1'
    int ltw_valid_flag;
    uint16_t ltw_offset;
    // piecewise_rate_flag == '1'
    uint32_t piecewise_rate;
    // seamless_splice_flag == '1'
    uint8_t splice_type;
    uint64_t dts_next_au;
} TSDAdaptationFieldExtension;

/**
 * Adaptation Field.
 */
typedef struct TSDAdaptationField {
    uint8_t adaptation_field_length;
    int flags;
    // PCR == '1'
    uint64_t program_clock_ref_base;
    uint16_t program_clock_ref_ext;
    // OPCR == '1'
    uint64_t orig_program_clock_ref_base;
    uint16_t orig_program_clock_ref_ext;
    // splicing_point_fag == '1'
    uint8_t splice_countdown;
    // transport private data flag == '1'
    uint8_t transport_private_data_length;
    const uint8_t *private_data_bytes;
    // adaptation_field_extension_flag == '1'
    TSDAdaptationFieldExtension adap_field_ext;
} TSDAdaptationField;

/**
 * Transport Stream Packet Header.
 */
typedef struct TSDPacket {
    uint8_t sync_byte;
    int flags;
    uint16_t pid;
    TSDScramblingControl transport_scrambling_control;
    TSDAdaptionFieldControl adaptation_field_control;
    uint8_t continuity_counter;
    TSDAdaptationField adaptation_field;
    const uint8_t *data_bytes;
    size_t data_bytes_length;
} TSDPacket;

/**
 * DSM Trick Mode.
 */
typedef struct TSDDSMTrickMode {
    TSDTrickModeControl control;
    uint8_t field_id;
    uint8_t intra_slice_refresh;
    uint8_t frequency_truncation;
    uint8_t rep_cntrl;
} TSDDSMTrickMode;

/**
 * System Header Stream.
 */
typedef struct TSDSystemHeaderStream {
    uint8_t stream_id;
    uint8_t pstd_buffer_bound_scale;
    uint16_t pstd_buffer_size_bound;
} TSDSystemHeaderStream;

/**
 * Program Stream System Header.
 */
typedef struct TSDSystemHeader {
    uint32_t start_code;
    uint16_t length;
    uint32_t rate_bound;
    uint8_t audio_bound;
    int flags;
    uint8_t video_bound;
    size_t stream_count;
    TSDSystemHeaderStream *streams;
} TSDSystemHeader;

/**
 * Program Stream Pack Header.
 */
typedef struct TSDPackHeader {
    uint8_t length;
    uint32_t start_code;
    uint64_t system_clock_ref_base;
    uint16_t system_clock_ref_ext;
    uint32_t program_mux_rate;
    uint8_t stuffing_length;
    TSDSystemHeader system_header;
} TSDPackHeader;

/**
 * PES Extension.
 */
typedef struct TSDPESExtension {
    int flags;
    uint8_t pes_private_data[16]; // 128 bits
    TSDPackHeader pack_header;
    uint8_t program_packet_sequence_counter;
    int mpeg1_mpeg2_identifier;
    uint8_t original_stuff_length;
    int pstd_buffer_scale;
    uint16_t pstd_buffer_size;
    uint8_t pes_extension_field_length;
} TSDPESExtension;

/**
 * PES Packet.
 */
typedef struct TSDPESPacket {
    uint32_t start_code;
    uint8_t stream_id;
    uint16_t packet_length;
    TSDPESScramblingControl scrambling_control;
    int flags;
    uint8_t header_data_length;
    uint64_t pts;
    uint64_t dts;
    uint64_t escr;
    uint16_t escr_extension;
    uint32_t es_rate;
    TSDDSMTrickMode trick_mode;
    uint8_t additional_copy_info;
    uint16_t previous_pes_packet_crc;
    TSDPESExtension extension;
    const uint8_t *data_bytes;
    size_t data_bytes_length;
} TSDPESPacket;

// re-typing the PESPacket for user callback consistency.
typedef TSDPESPacket TSDPESData;

/**
 * TSDTable Section.
 * Represents any short or long form table section, both PSI and private.
 */
typedef struct TSDTableSection {
    uint8_t table_id;
    int flags;
    uint16_t section_length;
    uint16_t table_id_extension;
    uint8_t version_number;
    uint8_t section_number;
    uint8_t last_section_number;
    uint32_t crc_32;
    uint8_t *section_data;
    size_t section_data_length;
} TSDTableSection;

/**
 * TSDTable.
 * Represents a table which is made up of multiple sections.
 */
typedef struct TSDTable {
    TSDTableSection *sections;
    size_t length;
} TSDTable;

/**
 * PAT Data.
 * PAT Data extracted from PAT TSDTable Sections.
 */
typedef struct TSDPATData {
    uint16_t *program_number;
    uint16_t *pid;
    size_t length;
} TSDPATData;

/**
 * PMT TSDDescriptor.
 * Outter or Inner TSDDescriptor found within the PMT.
 */
typedef struct TSDDescriptor {
    uint8_t tag;
    uint8_t length;
    const uint8_t *data;
    size_t data_length;
} TSDDescriptor;

/**
 * Program Element.
 * Description of a Program from the PMT.
 */
typedef struct TSDProgramElement {
    uint8_t stream_type;
    uint16_t elementary_pid;
    uint16_t es_info_length;
    TSDDescriptor *descriptors;
    size_t descriptors_length;
} TSDProgramElement;

/**
 * PMT Data.
 * PMT Data extracted from PMT TSDTable Sections.
 */
typedef struct TSDPMTData {
    uint16_t pcr_pid;
    uint16_t program_info_length;
    TSDDescriptor *descriptors;
    size_t descriptors_length;
    TSDProgramElement *program_elements;
    size_t program_elements_length;
    uint32_t crc_32;
} TSDPMTData;

/**
 * Descritor Data.
 * TSDDescriptor Data extracted from TSDTable Sections.
 */
typedef struct TSDDescriptorData {
    TSDDescriptor *descriptors;
    size_t descriptors_length;
} TSDDescriptorData;

typedef TSDDescriptorData TSDCATData;
typedef TSDDescriptorData TSDTSDTData;

/**
 * TSDTable Data.
 * Raw TSDTable Data extracted from a TSDTable which we don't support the parsing of.
 */
typedef struct TSDTableData {
    TSDTable *table;   /// TSDTable section information
    uint8_t *data;  /// Contiguous block of the table section data
    size_t size;    /// The number of bytes in data
} TSDTableData;

/**
 * TS Demux Registration.
 * Lists what data of data the user wants to listen out for.
 */
typedef struct TSDemuxRegistration {
    uint16_t pid;
    int data_types;
} TSDemuxRegistration;

/**
 * TS Demux Context.
 * The TSDEmuxContext is used to separate multiple demux tasks.
 */
typedef struct TSDemuxContext {
    tsd_malloc malloc;
    tsd_realloc realloc;
    tsd_calloc calloc;
    tsd_free free;

    /**
     * Registerd PIDs.
     * The PIDs registered for demuxing.
     */
    TSDemuxRegistration registered_pids[TSD_MAX_PID_REGS];
    TSDDataContext *registered_pids_data[TSD_MAX_PID_REGS];
    size_t registered_pids_length;

    /**
     * On Event Callback.
     * User specified event Callback.
     */
    tsd_on_event event_cb;

    /**
     * PAT Data.
     */
    struct {
        TSDPATData value;
        int valid;
    } pat;

    /**
     * Data Context Buffers.
     * Tempoary pool of buffers used during demuxing.
     */
    struct {
        TSDDataContext *active;
        TSDDataContext *pool;
        size_t length;
    } buffers;

} TSDemuxContext;

/**
 * Video Stream Descriptor.
 */
typedef struct TSDDescriptorVideoStream {
    uint8_t tag;
    uint8_t length;
    int flags;
    uint8_t frame_rate_code;
    // if MPEG_1_only_flag == '0'
    uint8_t profile_and_level_indication;
    uint8_t chroma_format;
} TSDDescriptorVideoStream;

/**
 * Audio Stream Descriptor.
 */
typedef struct TSDDescriptorAudioStream {
    uint8_t tag;
    uint8_t length;
    int flags;
    uint8_t layer;
} TSDDescriptorAudioStream;

/**
 * Hierarchy Descriptor.
 */
typedef struct TSDDescriptorHierarchy {
    uint8_t tag;
    uint8_t length;
    uint8_t type;
    uint8_t layer_index;
    uint8_t embedded_layer_index;
    uint8_t channel;
} TSDDescriptorHierarchy;

/**
 * Registration Descriptor.
 */
typedef struct TSDDescriptorRegistration {
    uint8_t tag;
    uint8_t length;
    uint32_t format_identifier;
    const uint8_t *additional_id_info;
    size_t additional_id_info_length;
} TSDDescriptorRegistration;

/**
 * Data Stream Descriptor.
 */
typedef struct TSDDescriptorDataStreamAlignment {
    uint8_t tag;
    uint8_t length;
    uint8_t type;
} TSDDescriptorDataStreamAlignment;

/**
 * Target Background Grid Descriptor.
 */
typedef struct TSDDescriptorTargetBackgroundGrid {
    uint8_t tag;
    uint8_t length;
    uint16_t horizontal_size;
    uint16_t vertical_size;
    uint8_t aspect_ratio_info;
} TSDDescriptorTargetBackgroundGrid;

/**
 * Video Window Descriptor.
 */
typedef struct TSDDescriptorVideoWindow {
    uint8_t tag;
    uint8_t length;
    uint16_t horizontal_offset;
    uint16_t vertical_offset;
    uint8_t window_priority;
} TSDDescriptorVideoWindow;

/**
 * Conditional Access Descriptor.
 */
typedef struct TSDDescriptorConditionalAccess {
    uint8_t tag;
    uint8_t length;
    uint16_t ca_system_id;
    uint16_t ca_pid;
    const uint8_t *private_data_bytes;
    size_t private_data_bytes_length;
} TSDDescriptorConditionalAccess;

/**
 * ISO 639 Language Descriptor.
 */
typedef struct TSDDescriptorISO639Language {
    uint8_t tag;
    uint8_t length;
    // a single packet could hold up to 45 max,.
    uint32_t iso_language_code[45];
    uint8_t audio_type[45];
    size_t language_length;
} TSDDescriptorISO639Language;

/**
 *  System Clock Descriptor.
 */
typedef struct TSDDescriptorSystemClock {
    uint8_t tag;
    uint8_t length;
    uint8_t external_clock_reference_indicator;
    uint8_t clock_accuracy_integer;
    uint8_t clock_accuracy_exponent;
} TSDDescriptorSystemClock;

/**
 *  Multiplex Buffer Utilization Descriptor.
 */
typedef struct TSDDescriptorMultiplexBufferUtilization {
    uint8_t tag;
    uint8_t length;
    uint8_t bound_valid_flag;
    uint16_t ltw_offset_lower_bound;
    uint16_t ltw_offset_upper_bound;
} TSDDescriptorMultiplexBufferUtilization;

/**
 * Copyright Descriptor.
 */
typedef struct TSDDescriptorCopyright {
    uint8_t tag;
    uint8_t length;
    uint32_t identifier;
    const uint8_t *additional_copy_info;
    size_t additional_copy_info_length;
} TSDDescriptorCopyright;

/**
 * Maximum Bit-rate Descriptor.
 */
typedef struct TSDDescriptorMaxBitrate {
    uint8_t tag;
    uint8_t length;
    uint32_t max_bitrate;
} TSDDescriptorMaxBitrate;

/**
 * Private Data Indicator Descriptor.
 */
typedef struct TSDDescriptorPrivDataInd {
    uint8_t tag;
    uint8_t length;
    uint32_t private_data_indicator;
} TSDDescriptorPrivDataInd;

/**
 * Smoothing Buffer Descriptor.
 */
typedef struct TSDDescriptorSmoothingBuffer {
    uint8_t tag;
    uint8_t length;
    uint32_t sb_leak_rate;
    uint32_t sb_size;
} TSDDescriptorSmoothingBuffer;

/**
 * System Target Decoder (STD) Descriptor.
 */
typedef struct TSDDescriptorSysTargetDecoder {
    uint8_t tag;
    uint8_t length;
    uint8_t leak_valid_flag;
} TSDDescriptorSysTargetDecoder;

/**
 * IBP Descriptor.
 */
typedef struct TSDDescriptorIBP {
    uint8_t tag;
    uint8_t length;
    uint8_t closed_gop_flag;
    uint8_t identical_gop_flag;
    uint16_t max_gop_length;
} TSDDescriptorIBP;

/**
 * MPEG-4 Video Descriptor.
 */
typedef struct TSDDescriptorMPEG4Video {
    uint8_t tag;
    uint8_t length;
    uint8_t visual_profile_and_level;
} TSDDescriptorMPEG4Video;

/**
 * MPEG-4 Audio Descriptor.
 */
typedef struct TSDDescriptorMPEG4Audio {
    uint8_t tag;
    uint8_t length;
    uint8_t audio_profile_and_level;
} TSDDescriptorMPEG4Audio;

/**
 * IOD Descriptor.
 */
typedef struct TSDDescriptorIOD {
    uint8_t tag;
    uint8_t length;
    uint8_t scope_of_iod_label;
    uint8_t iod_label;
    const uint8_t *initial_object_descriptor;
    size_t initial_object_descriptor_length;
} TSDDescriptorIOD;

/**
 * SL Descriptor.
 */
typedef struct TSDDescriptorSL {
    uint8_t tag;
    uint8_t length;
    uint16_t es_id;
} TSDDescriptorSL;

/**
 * FMC Descriptor.
 */
typedef struct TSDDescriptorFMC {
    uint8_t tag;
    uint8_t length;
    uint16_t es_id[32];
    uint8_t flex_mux_channel[32];
    size_t fmc_length;
} TSDDescriptorFMC;

/**
 * External ES ID Descriptor.
 */
typedef struct TSDDescriptorExternalESID {
    uint8_t tag;
    uint8_t length;
    uint16_t es_id;
} TSDDescriptorExternalESID;

/**
 * MuxCode Descriptor.
 */
typedef struct TSDDescriptorMuxCode {
    uint8_t tag;
    uint8_t length;
    const uint8_t *mux_code_table_entries;
    size_t mux_code_table_entries_length;
} TSDDescriptorMuxCode;

/**
 * FMX Buffer Size Descriptor.
 */
typedef struct TSDDescriptorFMXBufferSize {
    uint8_t tag;
    uint8_t length;
    const uint8_t *default_flex_mux_buffer_descriptor;
    size_t default_flex_mux_buffer_descriptor_length;
    const uint8_t *flex_mux_buffer_descriptors;
    size_t flex_mux_buffer_descriptors_length;
} TSDDescriptorFMXBufferSize;

/**
 * Multiplex Buffer Descriptor.
 */
typedef struct TSDDescriptorMultiplexBuffer {
    uint8_t tag;
    uint8_t length;
    uint32_t mb_buffer_size;
    uint32_t tb_leak_rate;
} TSDDescriptorMultiplexBuffer;

/**
 * Get software version.
 * Gets the verison of the softare as a string.
 * The format of the version is MAJOR.MINOR.PATCH where each of the
 * sections are an integer.
 * @return The version of the software as a string.
 */
const char* tsd_get_version(void);

/**
 * Initializes a Demux Context.
 * Sets default options and initializes a TSDemuxContext.
 * TSDemuxContext must be a valid pointer to a TSDemuxContext object.
 * @param ctx The TSDemuxContext to initialize.
 * @return TSD_OK on success.
 */
TSDCode tsd_context_init(TSDemuxContext *ctx);

/**
 * Destroys a previously initialized Demux Context.
 * @param ctx THe TSDemuxContext to destroy.
 * @return TSD_OK on success.
 */
TSDCode tsd_context_destroy(TSDemuxContext *ctx);

/**
 * Set Demux Context's Event Callback.
 * Sets the callback for all events that occur during demuxing.
 * Setting callback to NULL will remove an existing callback.
 * The callback may be set during the demux process.
 * @param ctx The Context to set the callback for.
 * @param callback The callback function that will be called when an
 *                 event occurs during the demux process. Set this to
 *                 NULL to remove the current callback.
 * @return Returns TSD_OK on success.
 */
TSDCode tsd_set_event_callback(TSDemuxContext *ctx, tsd_on_event callback);

/**
 * Demux a Transport Stream.
 * @param ctx The contenxt being used to demux,
 * @param data The data to demux.
 * @param size The size of data.
 * @param parsedSize The total number of bytes parsed will be populated in parsedSize. This will be 0 if an error is returned.
 * @return Returns TSD_OK on success.
 */
TSDCode tsd_demux(TSDemuxContext *ctx, void *data, size_t size, size_t *parsedSize);

/**
 * Ends the Demuxxing process.
 * Flushing any pending PES packets in the buffers.
 * @param ctx The context being used to demux.
 * @return TSD_OK on success.
 */
TSDCode tsd_demux_end(TSDemuxContext *ctx);

/**
 * Parse Packet Header.
 * Parses a TS Packet from the supplied data.
 * The parsed output is put into the hdr parameter supplied.
 * The hdr pointer must be a pointer to a valid TSDPacket object.
 * @param ctx The context being used to demux.
 * @param data The TS data to parse.
 * @param size The number of bytes to parse.
 * @param hdr The ouput of the parsing will do into the object referenced by
 *            this pointer.
 * @return TSD_OK on success. See the TSDCode enum for error codes.
 */
TSDCode tsd_parse_packet_header(TSDemuxContext *ctx,
                                const uint8_t *data,
                                size_t size,
                                TSDPacket *hdr);

/**
 * Parse TS Packet Adaption Field.
 * Parses the Adaption Field found inside a TSDPacket.
 * This function is called internally when parsing TS Packets.
 * @param ctx The context being used to demux.
 * @param data The data to parse.
 * @param size The size of the data to parse.
 * @param adap The AdaptatioField object that will store the result.
 * @return TSD_OK on success.
 */
TSDCode tsd_parse_adaptation_field(TSDemuxContext *ctx,
                                   const uint8_t *data,
                                   size_t size,
                                   TSDAdaptationField *adap);

/**
 * Parses TSDTable packets.
 * Parses a series of packets to construct a generic table. A TSDTable
 * can be contained within a single Packet or across multiple.
 * The data contained within the table to produce a PAT, PMT or CAT
 * needs to be parsed once the generic table has been parsed.
 * This function supports both short and long form tables.
 * @param ctx The context being used to demux.
 * @param pkt The packet to parse.
 * @param table Where to store the table output.
 * @return TSD_OK will be returned on successful parsing of a table
 *                TSD_INCOMPLETE_TABLE will be returned when there is
 *                not enough data to complete the table. tsd_parse_table
 *                will then need to be called with the next packets
 *                idenitfied with the sample table PID.
 */

TSDCode tsd_parse_table(TSDemuxContext *ctx,
                        TSDPacket *pkt,
                        TSDTable *table);

/**
 * Parses all the TSDTable Sections to form a TSDTable.
 * Takes complete TSDTable data, which is all the TSDTable Sections that
 * make up a table, and parses them into the supplied TSDTable object.
 * This is called internally by tsd_parse_table.
 * @param ctx The context being used to demux.
 * @param data The raw TSDTable data to parse.
 * @param size The number of bytes that make up the table.
 * @param table The TSDTable where the TableSections will be stored.
 * @return TSD_OK on success.
 */
TSDCode tsd_parse_table_sections(TSDemuxContext *ctx,
                                 uint8_t *data,
                                 size_t size,
                                 TSDTable *table);

/**
 * Parses PAT data from a TSDTable.
 * The table data is the data supplied by a TSDTable object once a
 * generic table has been processed using tsd_parse_table.
 * @param ctx The context being used to demux.
 * @param data The table data to parse into PAT values.
 * @param size The size of the table data. Should be in multiple of 4
 *             bytes
 * @param pat The TSDPATData that will store the result.
 * @return Returns TSD_OK on success.
 */
TSDCode tsd_parse_pat(TSDemuxContext *ctx,
                      const uint8_t *data,
                      size_t size,
                      TSDPATData* pat);

/**
 * Parses TSDDescriptor data from a TSDTable.
 * The table data is the data supplied by a TSDTable object once a
 * generic table has been processed using tsd_parse_table.
 * @param ctx The context being used to demux.
 * @param data The table data to parse into CAT values.
 * @param size The size of the table data.
 * @param descriptorData The TSDDescriptorData that will store the result.
 * @return Returns TSD_OK on success.
 */
TSDCode tsd_parse_descriptors(TSDemuxContext *ctx,
                              const uint8_t *data,
                              size_t size,
                              TSDDescriptorData *descriptorData);

/**
 * Parses PMT data from a TSDTable.
 * The table data is the data supploed by a TSDTable object once a
 * generic table has been processed using tsd_parse_table.
 * @param ctx The context being used to demux.
 * @param data The table data to parse into a PMT.
 * @param size The size of the table data.
 * @param pmt The TSDPMTData that will store the result.
 * @return Returns TSD_OK on success.
 */
TSDCode tsd_parse_pmt(TSDemuxContext *ctx,
                      const uint8_t *data,
                      size_t size,
                      TSDPMTData *pmt);

/**
 * Parses PES packets.
 * @param ctx The context being used to demux.
 * @param data The raw data to parse.
 * @param size The size of the data.
 * @param pes The TSDPESPacket that the data will be parsed into.
 * @return Returns TSD_OK on success.
 */
TSDCode tsd_parse_pes(TSDemuxContext *ctx,
                      const uint8_t *data,
                      size_t size,
                      TSDPESPacket *pes);

/**
 * Parses and Extracts data from a TSDTable.
 * Parses packets that make up a table and Extracts the data into a contiguous
 * memory buffer.
 * @param ctx The context being used to demux.
 * @param hdr The next packet to parse
 * @param table A pointer to a TSDTable Structure that will be populated with the
 *              table section information.
 * @param mem A pointer used to store the memry buffer location.
 * @param size A pointer to an integer used to store the number of bytes written
 *             to the memory buffer.
 * @return TSD_OK once the table is parsed completely. TSD_INCOMPLETE_TABLE if
 *         the table is incomplete and requires more packets to be parsed.
 *         Any other response must be treated as an error.
 */
TSDCode tsd_table_data_extract(TSDemuxContext *ctx,
                               TSDPacket *hdr,
                               TSDTable *table,
                               uint8_t **mem,
                               size_t *size);

/**
 * Converts raw Decriptor data into a TSDDescriptor array.
 * @param ctx The context being used to demux.
 * @param data The raw Descriptor data.
 * @param data_size The size of the Descriptor data.
 * @param descriptors Pointer to assign the array of TSDDescriptor to.
 * @param descriptors_length Where to write the number of Descriptors available in
 *        descriptors.
 * @return TSD_OK on success.
 */
TSDCode tsd_descriptor_extract(TSDemuxContext *ctx,
                               const uint8_t *data,
                               size_t data_size,
                               TSDDescriptor **descriptors,
                               size_t *descriptors_length);

/**
 * Destroys Table Data.
 * Detroys data used in a Table.
 * @param ctx The context being used to demux.
 * @param table The table to destroy.
 * @returns TSD_OK on success.
 */
TSDCode tsd_table_data_destroy(TSDemuxContext *ctx, TSDTable *table);

/**
 * Data Content Initializtion.
 * Initializes a Data Context. Do not call multiple times on the same Context
 * unless the context has been destroyed.
 * A TSDDataContext must be initialized before being used anywhere in the API.
 * @param ctx The context being used to demux.
 * @param dataCtx The TSDDataContext to initialize.
 * @return TSD_OK on succes.
 */
TSDCode tsd_data_context_init(TSDemuxContext *ctx, TSDDataContext *dataCtx);

/**
 * Destroys a TSDDataContext.
 * Destroys a TSDDataContext that has been previously initialized.
 * Calling tsd_data_context_destroy multiple times after initializing a TSDDataContext
 * will not cause any problems.
 * Do not call tsd_data_context_destroy on an unitialized TSDDataContext, unexecpted
 * behavior will occur.
 * It is possible to reinitialize a TSDDataContext once it has been destroyed.
 * @param ctx The context being used to demux.
 * @param dataCtx The TSDDataContext to destroy.
 * @return TSD_OK on success and if the TSDDataContext has already been destroyed.
 */
TSDCode tsd_data_context_destroy(TSDemuxContext *ctx, TSDDataContext *dataCtx);

/**
 * Writes data to TSDDataContext.
 * The TSDDataContext will dynamically allocate more memory if there is not enough
 * space in the TSDDataContext buffer.
 * Supplying NULL data or a size of zero will cause tsd_data_context_write to return
 * an error.
 * @param ctx The context being used to demux.
 * @param dataCtx The TSDDataContext to write the data to.
 * @param data The data to write.
 * @param size The number of bytes to write.
 * @returns TSD_OK on success.
 */
TSDCode tsd_data_context_write(TSDemuxContext *ctx, TSDDataContext *dataCtx, const uint8_t *data, size_t size);

/**
 * Reset Data Context.
 * Resets the write buffer effecively clearing the buffer and starting over.
 * Re-uses the existing buffer even if it has previously been dynamically
 * allocated during a write process.
 * @param ctx The context being used to demux.
 * @param dataCtx The TSDDataContext to reset.
 * @return TSD_OK on success.
 */
TSDCode tsd_data_context_reset(TSDemuxContext *ctx, TSDDataContext *dataCtx);

/**
 * Register a PID for demuxing.
 * When a PID is registered, the user supplied callback will be called with the
 * data associated with that PID as packets are being parsed.
 * @param ctx The context being used to demux.
 * @param pid The PID being registered.
 * @param reg_data_type What type of data to register.
 * @return TSD_OK on success.
 * @see TSDRegType
 */
TSDCode tsd_register_pid(TSDemuxContext *ctx, uint16_t pid, int reg_data_type);

/**
 * Deregisters a PID for demuxing.
 * Removes a PID from the PID demuxing register.
 * @param ctx The context being used to demux.
 * @param pid The PID being deregistered.
 * @return TSD_OK on success.
 */
TSDCode tsd_deregister_pid(TSDemuxContext *ctx, uint16_t pid);

/**
 * Parses a Video Stream Descriptor.
 * @param data The data to parse.
 * @param size, The size of the data in bytes.
 * @param desc The descriptor to write the parsed information into.
 * @return TSD_OK on success.
 */
TSDCode tsd_parse_descriptor_video_stream(const uint8_t *data,
        size_t size,
        TSDDescriptorVideoStream *desc);

/**
 * Parses an Audio Stream Descriptor.
 * @param data The data to parse.
 * @param size, The size of the data in bytes.
 * @param desc The descriptor to write the parsed information into.
 * @return TSD_OK on success.
 */
TSDCode tsd_parse_descriptor_audio_stream(const uint8_t *data,
        size_t size,
        TSDDescriptorAudioStream *desc);

/**
 * Parses a Hierarchy Descriptor.
 * @param data The data to parse.
 * @param size The size of the data in bytes.
 * @param desc The descriptor to write the parsed information into.
 * @return TSD_OK on success.
 */
TSDCode tsd_parse_descriptor_hierarchy(const uint8_t *data,
                                       size_t size,
                                       TSDDescriptorHierarchy *desc);
/**
 * Parses a Registration Descriptor.
 * @param data The data to parse.
 * @param size The size of the data in bytes.
 * @param desc The descriptor to write the parsed information into.
 * @return TSD_OK on success.
 */
TSDCode tsd_parse_descriptor_registration(const uint8_t *data,
        size_t size,
        TSDDescriptorRegistration *desc);

/**
 * Parses a Registration Descriptor.
 * @param data The data to parse.
 * @param size The size of the data in bytes.
 * @param desc The descriptor to write the parsed information into.
 * @return TSD_OK on success.
 */
TSDCode tsd_parse_descriptor_registration(const uint8_t *data,
        size_t size,
        TSDDescriptorRegistration *desc);

/**
 * Parses a Data Stream Alignment Descriptor.
 * @param data The data to parse.
 * @param size The size of the data in bytes.
 * @param desc The descriptor to write the parsed information into.
 * @return TSD_OK on success.
 */
TSDCode tsd_parse_descriptor_data_stream_alignment(const uint8_t *data,
        size_t size,
        TSDDescriptorDataStreamAlignment *desc);

/**
 * Parses a Target Background Grid Descriptor.
 * @param data The data to parse.
 * @param size The size of the data in bytes.
 * @param desc The descriptor to write the parsed information into.
 * @return TSD_OK on success.
 */
TSDCode tsd_parse_descriptor_target_background_grid(const uint8_t *data,
        size_t size,
        TSDDescriptorTargetBackgroundGrid *desc);

/**
 * Parses a Video Window Descriptor.
 * @param data The data to parse.
 * @param size The size of the data in bytes.
 * @param desc The descriptor to write the parsed information into.
 * @return TSD_OK on success.
 */
TSDCode tsd_parse_descriptor_video_window(const uint8_t *data,
        size_t size,
        TSDDescriptorVideoWindow *desc);

/**
 * Parses a Conditional Access Descriptor.
 * @param data The data to parse.
 * @param size The size of the data in bytes.
 * @param desc The descriptor to write the parsed information into.
 * @return TSD_OK on success.
 */
TSDCode tsd_parse_descriptor_conditional_access(const uint8_t *data,
        size_t size,
        TSDDescriptorConditionalAccess *desc);

/**
 * Parses a ISO 639 Language Descriptor.
 * @param data The data to parse.
 * @param size The size of the data in bytes.
 * @param desc The descriptor to write the parsed information into.
 * @return TSD_OK on success.
 */
TSDCode tsd_parse_descriptor_iso639_language(const uint8_t *data,
        size_t size,
        TSDDescriptorISO639Language *desc);

/**
 * Parses a System Clock Descriptor.
 * @param data The data to parse.
 * @param size The size of the data in bytes.
 * @param desc The descriptor to write the parsed information into.
 * @return TSD_OK on success.
 */
TSDCode tsd_parse_descriptor_system_clock(const uint8_t *data,
        size_t size,
        TSDDescriptorSystemClock *desc);

/**
 * Parses a Multiplex Buffer Utilization Descriptor.
 * @param data The data to parse.
 * @param size The size of the data in bytes.
 * @param desc The descriptor to write the parsed information into.
 * @return TSD_OK on success.
 */
TSDCode tsd_parse_descriptor_multiplex_buffer_utilization(const uint8_t *data,
        size_t size,
        TSDDescriptorMultiplexBufferUtilization *desc);

/**
 * Parses a Copyright Descriptor.
 * @param data The data to parse.
 * @param size The size of the data in bytes.
 * @param desc The descriptor to write the parsed information into.
 * @return TSD_OK on success.
 */
TSDCode tsd_parse_descriptor_copyright(const uint8_t *data,
                                       size_t size,
                                       TSDDescriptorCopyright *desc);

/**
 * Parses a Maximum Bit-rate Descriptor.
 * @param data The data to parse.
 * @param size The size of the data in bytes.
 * @param desc The descriptor to write the parsed information into.
 * @return TSD_OK on success.
 */
TSDCode tsd_parse_descriptor_max_bitrate(const uint8_t *data,
        size_t size,
        TSDDescriptorMaxBitrate *desc);

/**
 * Parses a Private Data Indicator Descriptor.
 * @param data The data to parse.
 * @param size The size of the data in bytes.
 * @param desc The descriptor to write the parsed information into.
 * @return TSD_OK on success.
 */
TSDCode tsd_parse_descriptor_priv_data_ind(const uint8_t *data,
        size_t size,
        TSDDescriptorPrivDataInd *desc);

/**
 * Parses a Smoothing Buffer Descriptor.
 * @param data The data to parse.
 * @param size The size of the data in bytes.
 * @param desc The descriptor to write the parsed information into.
 * @return TSD_OK on success.
 */
TSDCode tsd_parse_descriptor_smoothing_buffer(const uint8_t *data,
        size_t size,
        TSDDescriptorSmoothingBuffer *desc);

/**
 * Parses a System Target Decoder Descriptor.
 * @param data The data to parse.
 * @param size The size of the data in bytes.
 * @param desc The descriptor to write the parsed information into.
 * @return TSD_OK on success.
 */
TSDCode tsd_parse_descriptor_sys_target_decoder(const uint8_t *data,
        size_t size,
        TSDDescriptorSysTargetDecoder *desc);

/**
 * Parses a IBP Descriptor.
 * @param data The data to parse.
 * @param size The size of the data in bytes.
 * @param desc The descriptor to write the parsed information into.
 * @return TSD_OK on success.
 */
TSDCode tsd_parse_descriptor_ibp(const uint8_t *data,
                                 size_t size,
                                 TSDDescriptorIBP *desc);

/**
 * Parses a MPEG-4 Video Descriptor.
 * @param data The data to parse.
 * @param size The size of the data in bytes.
 * @param desc The descriptor to write the parsed information into.
 * @return TSD_OK on success.
 */
TSDCode tsd_parse_descriptor_mpeg4_video(const uint8_t *data,
        size_t size,
        TSDDescriptorMPEG4Video *desc);

/**
 * Parses a MPEG-4 Audio Descriptor.
 * @param data The data to parse.
 * @param size The size of the data in bytes.
 * @param desc The descriptor to write the parsed information into.
 * @return TSD_OK on success.
 */
TSDCode tsd_parse_descriptor_mpeg4_audio(const uint8_t *data,
        size_t size,
        TSDDescriptorMPEG4Audio *desc);

/**
 * Parses a IOD Descriptor.
 * @param data The data to parse.
 * @param size The size of the data in bytes.
 * @param desc The descriptor to write the parsed information into.
 * @return TSD_OK on success.
 */
TSDCode tsd_parse_descriptor_iod(const uint8_t *data,
                                 size_t size,
                                 TSDDescriptorIOD *desc);

/**
 * Parses a SL Descriptor.
 * @param data The data to parse.
 * @param size The size of the data in bytes.
 * @param desc The descriptor to write the parsed information into.
 * @return TSD_OK on success.
 */
TSDCode tsd_parse_descriptor_sl(const uint8_t *data,
                                size_t size,
                                TSDDescriptorSL *desc);

/**
 * Parses a FMC Descriptor.
 * @param data The data to parse.
 * @param size The size of the data in bytes.
 * @param desc The descriptor to write the parsed information into.
 * @return TSD_OK on success.
 */
TSDCode tsd_parse_descriptor_fmc(const uint8_t *data,
                                 size_t size,
                                 TSDDescriptorFMC *desc);

/**
 * Parses a External ES ID Descriptor.
 * @param data The data to parse.
 * @param size The size of the data in bytes.
 * @param desc The descriptor to write the parsed information into.
 * @return TSD_OK on success.
 */
TSDCode tsd_parse_descriptor_external_es_id(const uint8_t *data,
        size_t size,
        TSDDescriptorExternalESID *desc);

/**
 * Parses a MuxCode Descriptor.
 * @param data The data to parse.
 * @param size The size of the data in bytes.
 * @param desc The descriptor to write the parsed information into.
 * @return TSD_OK on success.
 */
TSDCode tsd_parse_descriptor_mux_code(const uint8_t *data,
                                      size_t size,
                                      TSDDescriptorMuxCode *desc);

/**
 * Parses a FMX Buffer Size Descriptor.
 * @param data The data to parse.
 * @param size The size of the data in bytes.
 * @param desc The descriptor to write the parsed information into.
 * @return TSD_OK on success.
 */
TSDCode tsd_parse_descriptor_fmx_buffer_size(const uint8_t *data,
        size_t size,
        TSDDescriptorFMXBufferSize *desc);

/**
 * Parses a Multiplex Buffer Descriptor.
 * @param data The data to parse.
 * @param size The size of the data in bytes.
 * @param desc The descriptor to write the parsed information into.
 * @return TSD_OK on success.
 */
TSDCode tsd_parse_descriptor_multiplex_buffer(const uint8_t *data,
        size_t size,
        TSDDescriptorMultiplexBuffer *desc);


#ifdef __cplusplus
}
#endif


#endif // TS_DEMUX_H
