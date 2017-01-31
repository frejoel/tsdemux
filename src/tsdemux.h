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
#define TSD_DEFAULT_DATA_CONTEXT_SIZE         (256)

/**
 * @file
 * @ingroup libts
 * Libts external API header
 */

// forward declarations
typedef struct TSDemuxContext TSDemuxContext;
typedef struct Table Table;
typedef struct TableSection TableSection;

typedef enum EventId EventId;

/**
 * Memory Allocator.
 * Allocate memory block.
 * See realloc C90 (C++98) definition.
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
                              EventId id,
                              void *data);

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
    TSD_INVALID_START_CODE_PREFIX             = 0x0006,
    TSD_OUT_OF_MEMORY                         = 0x0007,
    TSD_INCOMPLETE_TABLE                      = 0x0008,
    TSD_NOT_A_TABLE_PACKET                    = 0x0009,
    TSD_PARSE_ERROR                           = 0x000A,
} TSCode;

/**
 * Transport Stream Packet Flags.
 */
typedef enum TSPacketFlags {
    TSPF_TRANSPORT_ERROR_INDICATOR            = 0x04,
    TSPF_PAYLOAD_UNIT_START_INDICATOR         = 0x02,
    TSPF_TRANSPORT_PRIORITY                   = 0x01,
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
    /** DVB Service Information **/
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
    AF_DISCONTINUITY_INDICATOR                = 0x80,
    AF_RANDOM_ACCESS_INDICATOR                = 0x40,
    AF_ELEMENTARY_STREAM_PRIORIY_INDICATOR    = 0x20,
    AF_PCR_FLAG                               = 0x10,
    AF_OPCR_FLAG                              = 0x08,
    AF_SPLICING_POINT_FLAG                    = 0x04,
    AF_TRANSPORT_PRIVATE_DATA_FLAG            = 0x02,
    AF_ADAPTATION_FIELD_EXTENSION_FLAG        = 0x01,
} AdaptationFieldFlags;

/**
 * Adaptation Field Extensions Flags.
 */
typedef enum AdaptationFieldExtensionFlags {
    AFEF_LTW_FLAG                             = 0x04,
    AFEF_PIECEWISE_RATE_FLAG                  = 0x02,
    AFEF_SEAMLESS_SPLCE_FLAG                  = 0x01,
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
    SHF_SYSTEM_AUDIO_LOCK_FLAG                = 0x04,
    SHF_SYSTEM_VIDEO_LOCK_FLAG                = 0x08,
    SHF_PACKET_RATE_RESTICTION_FLAG           = 0x10,
} SystemHeaderFlags;

/**
 * Table Flags.
 */
typedef enum TableFlags {
    TBL_PRIVATE_INDICATOR                     = 0x01,
    TBL_SECTION_SYNTAX_INDICATOR              = 0x02,
    TBL_CURRENT_NEXT_INDICATOR                = 0x04,
} TableFlags;

/**
 * Event Id.
 * The Id of all events that may occur during demux.
 */
typedef enum EventId {
    TSD_EVENT_PAT                            = 0x0001,
    TSD_EVENT_PMT                            = 0x0002,
    TSD_EVENT_CAT                            = 0x0004,
    TSD_EVENT_TSDT                           = 0x0008,
    /// Unsupported Table
    TSD_EVENT_TABLE                          = 0x0010,
} EventId;

/**
 * Data Context.
 * Used to persist the session when streaming TS packets through the demux in
 * multiple calls.
 */
typedef struct DataContext {
    uint8_t *buffer;
    uint8_t *write;
    uint8_t *end;
    size_t size;
    uint32_t id;
} DataContext;

/**
 * Adaptation Field Extension.
 */
typedef struct AdaptationFieldExtension {
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
} AdaptationFieldExtension;

/**
 * Adaptation Field.
 */
typedef struct AdaptationField {
    uint8_t adaptation_field_length;
    int flags;
    // PCR == '1'
    uint64_t program_clock_reference_base;
    uint16_t program_clock_reference_extension;
    // OPCR == '1'
    uint64_t original_program_clock_reference_base;
    uint16_t original_program_clock_reference_extension;
    // splicing_point_fag == '1'
    uint8_t splice_countdown;
    // transport provate data flag == '1'
    uint8_t transport_private_data_length;
    const void *private_data_byte;
    // adaptation_field_extension_flag == '1'
    AdaptationFieldExtension adaptation_field_extension;
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
    const uint8_t *data_bytes;
    size_t data_bytes_length;
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
 * Program Stream Pack Header.
 */
typedef struct PackHeader {
    uint32_t pack_start_code;
    uint64_t system_clock_reference_base;
    uint16_t system_clock_reference_extension;
    uint32_t program_mux_rate;
    uint32_t system_header_start_code;
    SystemHeader system_header;
} PackHeader;

/**
 * PES Extension.
 */
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

/**
 * Table Section.
 * Represents any short or long form table section, both PSI and private.
 */
typedef struct TableSection {
    uint8_t table_id;
    int flags;
    uint16_t section_length;
    union {
        uint16_t pat_transport_stream_id;
        uint16_t pmt_program_number;
        uint16_t cat_reserved;
    } u16;
    uint8_t version_number;
    uint8_t section_number;
    uint8_t last_section_number;
    uint32_t crc_32;
    uint8_t *section_data;
    size_t section_data_length;
} TableSection;

/**
 * Table.
 * Represents a table which is made up of multiple sections.
 */
typedef struct Table {
    TableSection *sections;
    size_t length;
} Table;

/**
 * PAT Data.
 * PAT Data extracted from PAT Table Sections.
 */
typedef struct PATData {
    uint16_t *program_number;
    uint16_t *pid;
    size_t length;
} PATData;

/**
 * PMT Descriptor.
 * Outter or Inner Descriptor found within the PMT.
 */
typedef struct Descriptor {
    uint8_t tag;
    uint8_t length;
    const uint8_t *data;
} Descriptor;

/**
 * Program Element.
 * Description of a Program from the PMT.
 */
typedef struct ProgramElement {
    uint8_t stream_type;
    uint16_t elementary_pid;
    uint16_t es_info_length;
    Descriptor *descriptors;
    size_t descriptors_length;
} ProgramElement;

/**
 * PMT Data.
 * PMT Data extracted from PMT Table Sections.
 */
typedef struct PMTData {
    uint16_t pcr_pid;
    uint16_t program_info_length;
    Descriptor *descriptors;
    size_t descriptors_length;
    ProgramElement *program_elements;
    size_t program_elements_length;
    uint32_t crc_32;
} PMTData;

/**
 * CAT Data.
 * CAT Data extracted from CAT Table Sections.
 */
typedef struct CATData {
    Descriptor *descriptors;
    size_t descriptors_length;
} CATData;

/**
 * Table Data.
 * Raw Table Data extracted from a Table which we don't support the parsing of.
 */
typedef struct TableData {
    Table *table;   /// Table section information
    uint8_t *data;  /// Contiguous block of the table section data
    size_t size;    /// The number of bytes in data
} TableData;

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
     * On Event Callback.
     * User specified event Callback.
     */
    tsd_on_event event_cb;

    struct {
        PATData value;
        int valid;
    } pat;

    struct {
        PMTData *values;
        size_t length;
        size_t capacity;
    } pmt;

    struct {
        DataContext *active;
        DataContext *pool;
        size_t length;
    } buffers;

} TSDemuxContext;

/**
 * Set Default Context.
 * Sets default options onto the TSDemuxContext for convience.
 * TSDemuxContext must be a valid pointer to a TSDemuxContext object.
 * @param ctx The TSDemuxContext to set default parameters onto.
 * @return TSD_OK on success.
 */
TSCode set_default_context(TSDemuxContext *ctx);

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
TSCode set_event_callback(TSDemuxContext *ctx, tsd_on_event callback);

/**
 * Demux a Transport Stream.
 */
size_t demux(TSDemuxContext *ctx, void *data, size_t size, TSCode *code);

/**
 * Parse Packet Header.
 * Parses a TS Packet from the supplied data.
 * The parsed output is put into the hdr parameter supplied.
 * The hdr pointer must be a pointer to a valid TSPacket object.
 * @param ctx The context being used to demux.
 * @param data The TS data to parse.
 * @param size The number of bytes to parse.
 * @param hdr The ouput of the parsing will do into the object referenced by
 *            this pointer.
 * @return TSD_OK on success. See the TSCode enum for error codes.
 */
TSCode parse_packet_header(TSDemuxContext *ctx,
                           const void *data,
                           size_t size,
                           TSPacket *hdr);

/**
 * Parse TS Packet Adaption Field.
 * Parses the Adaption Field found inside a TSPacket.
 * This function is called internally when parsing TS Packets.
 * @param ctx The context being used to demux.
 * @param data The data to parse.
 * @param size The size of the data to parse.
 * @param adap The AdaptatioField object that will store the result.
 * @return TSD_OK on success.
 */
TSCode parse_adaptation_field(TSDemuxContext *ctx,
                              const void *data,
                              size_t size,
                              AdaptationField *adap);

/**
 * Parses Table packets.
 * Parses a series of packets to construct a generic table. A Table
 * can be contained within a single Packet or across multiple.
 * The data contained within the table to produce a PAT, PMT or CAT
 * needs to be parsed once the generic table has been parsed.
 * This function supports both short and long form tables.
 * @param ctx The context being used to demux.
 * @param pkt The packet to parse.
 * @param table Where to store the table output.
 * @return TSD_OK will be returned on successful parsing of a table
 *                TSD_INCOMPLETE_TABLE will be returned when there is
 *                not enough data to complete the table. parse_table
 *                will then need to be called with the next packets
 *                idenitfied with the sample table PID.
 */

TSCode parse_table(TSDemuxContext *ctx,
                   TSPacket *pkt,
                   Table *table);

/**
 * Parses all the Table Sections to form a Table.
 * Takes complete Table data, which is all the Table Sections that
 * make up a table, and parses them into the supplied Table object.
 * This is called internally by parse_table.
 * @param ctx The context being used to demux.
 * @param data The raw Table data to parse.
 * @param size The number of bytes that make up the table.
 * @param table The Table where the TableSections will be stored.
 * @return TSD_OK on success.
 */
TSCode parse_table_sections(TSDemuxContext *ctx,
                            uint8_t *data,
                            size_t size,
                            Table *table);

/**
 * Parses PAT data from a Table.
 * The table data is the data supplied by a Table object once a
 * generic table has been processed using parse_table.
 * @param ctx The context being used to demux.
 * @param data The table data to parse into PAT values.
 * @param size The size of the table data. Should be in multiple of 4
 *             bytes
 * @param pat The PATData that will store the result.
 * @return Returns TSD_OK on success.
 */
TSCode parse_pat(TSDemuxContext *ctx,
                 const uint8_t *data,
                 size_t size,
                 PATData* pat);

/**
 * Parses CAT data from a Table.
 * The table data is the data supplied by a Table object once a
 * generic table has been processed using parse_table.
 * @param ctx The context being used to demux.
 * @param data The table data to parse into CAT values.
 * @param size The size of the table data.
 * @param cat The CATData that will store the result.
 * @return Returns TSD_OK on success.
 */
TSCode parse_cat(TSDemuxContext *ctx,
                 const uint8_t *data,
                 size_t size,
                 CATData* cat);

/**
 * Parses PMT data from a Table.
 * The table data is the data supploed by a Table object once a
 * generic table has been processed using parse_table.
 * @param ctx The context being used to demux.
 * @param data The table data to parse into a PMT.
 * @param size The size of the table data.
 * @param pmt The PMTData that will store the result.
 * @return Returns TSD_OK on success.
 */
TSCode parse_pmt(TSDemuxContext *ctx,
                 const uint8_t *data,
                 size_t size,
                 PMTData *pmt);

TSCode parse_pes(TSDemuxContext *ctx,
                 const void *data,
                 size_t size,
                 PESPacket *pes);

/**
 * Parses and Extracts data from a Table.
 * Parses packets that make up a table and Extracts the data into a contiguous
 * memory buffer.
 * @param ctx The context being used to demux.
 * @param hdr The next packet to parse
 * @param table A pointer to a Table Structure that will be populated with the
 *              table section information.
 * @param mem A pointer used to store the memry buffer location.
 * @param size A pointer to an integer used to store the number of bytes written
 *             to the memory buffer.
 * @return TSD_OK once the table is parsed completely. TSD_INCOMPLETE_TABLE if
 *         the table is incomplete and requires more packets to be parsed.
 *         Any other response must be treated as an error.
 */
TSCode extract_table_data(TSDemuxContext *ctx,
                          TSPacket *hdr,
                          Table *table,
                          uint8_t **mem,
                          size_t *size);

/**
 * Data Content Initializtion.
 * Initializes a Data Context. Do not call multiple times on the same Context
 * unless the context has been destroyed.
 * A DataContext must be initialized before being used anywhere in the API.
 * @param ctx The context being used to demux.
 * @param dataCtx The DataContext to initialize.
 * @return TSD_OK on succes.
 */
TSCode data_context_init(TSDemuxContext *ctx, DataContext *dataCtx);

/**
 * Destroys a DataContext.
 * Destroys a DataContext that has been previously initialized.
 * Calling data_context_destroy multiple times after initializing a DataContext
 * will not cause any problems.
 * Do not call data_context_destroy on an unitialized DataContext, unexecpted
 * behavior will occur.
 * It is possible to reinitialize a DataContext once it has been destroyed.
 * @param ctx The context being used to demux.
 * @param dataCtx The DataContext to destroy.
 * @return TSD_OK on success and if the DataContext has already been destroyed.
 */
TSCode data_context_destroy(TSDemuxContext *ctx, DataContext *dataCtx);

/**
 * Writes data to DataContext.
 * The DataContext will dynamically allocate more memory if there is not enough
 * space in the DataContext buffer.
 * Supplying NULL data or a size of zero will cause data_context_write to return
 * an error.
 * @param ctx The context being used to demux.
 * @param dataCtx The DataContext to write the data to.
 * @param data The data to write.
 * @param size The number of bytes to write.
 * @returns TSD_OK on success.
 */
TSCode data_context_write(TSDemuxContext *ctx, DataContext *dataCtx, const uint8_t *data, size_t size);

/**
 * Reset Data Context.
 * Resets the write buffer effecively clearing the buffer and starting over.
 * Re-uses the existing buffer even if it has previously been dynamically
 * allocated during a write process.
 * @param ctx The context being used to demux.
 * @param dataCtx The DataContext to reset.
 * @return TSD_OK on success.
 */
TSCode data_context_reset(TSDemuxContext *ctx, DataContext *dataCtx);

#endif // TS_DEMUX_H
