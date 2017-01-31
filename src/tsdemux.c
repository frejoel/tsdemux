#include "tsdemux.h"
#include "string.h"
#include <stdio.h>

uint16_t parse_uint16(uint16_t val)
{
#ifdef __BIG_ENDIAN__
    return val;
#else
    return ((val >> 8) & 0x00FF) | ((val << 8) & 0xFF00);
#endif
}

uint64_t parse_uint64(uint64_t val)
{
#ifdef __BIG_ENDIAN__
    return val;
#else
    return ((val >> 56) & 0x00000000000000FFL) |
           ((val >> 40) & 0x000000000000FF00L) |
           ((val >> 24) & 0x0000000000FF0000L) |
           ((val >>  8) & 0x00000000FF000000L) |
           ((val <<  8) & 0x000000FF00000000L) |
           ((val << 24) & 0x0000FF0000000000L) |
           ((val << 40) & 0x00FF000000000000L) |
           ((val << 56) & 0xFF00000000000000L) ;
#endif
}

uint64_t parse_uint32(uint32_t val)
{
#ifdef __BIG_ENDIAN__
    return val;
#else
    return ((val >> 24) & 0x000000FF) |
           ((val >>  8) & 0x0000FF00) |
           ((val <<  8) & 0x00FF0000) |
           ((val << 24) & 0xFF000000) ;
#endif
}

TSCode set_default_context(TSDemuxContext *ctx)
{
    if(ctx == NULL)                 return TSD_INVALID_CONTEXT;

    memset(ctx, 0, sizeof(TSDemuxContext));

    ctx->malloc = malloc;
    ctx->realloc = realloc;
    ctx->calloc = calloc;
    ctx->free = free;

    // initialize the user defined event callback
    ctx->event_cb = (tsd_on_event) NULL;

    return TSD_OK;
}

TSCode set_event_callback(TSDemuxContext *ctx, tsd_on_event callback)
{
    if(ctx == NULL)     return TSD_INVALID_CONTEXT;
    // Note that we allow the callback to be NULL
    ctx->event_cb = callback;
    return TSD_OK;
}

TSCode parse_packet_header(TSDemuxContext *ctx,
                           const void *data,
                           size_t size,
                           TSPacket *hdr)
{
    if(ctx == NULL)                 return TSD_INVALID_CONTEXT;
    if(data == NULL)                return TSD_INVALID_DATA;
    if(size < TSD_TSPACKET_SIZE)    return TSD_INVALID_DATA_SIZE;
    if(hdr == NULL)                 return TSD_INVALID_ARGUMENT;

    const uint8_t *ptr = (const uint8_t *)data;
    const uint8_t *end = &ptr[size];
    // check the sync byte
    if(ptr+1 > end) return TSD_INVALID_DATA_SIZE;
    hdr->sync_byte = *ptr;
    if(hdr->sync_byte != TSD_SYNC_BYTE)  return TSD_INVALID_SYNC_BYTE;
    ptr++;

    if(ptr+1 > end) return TSD_INVALID_DATA_SIZE;
    hdr->flags = (*ptr) >> 5;
    hdr->pid = parse_uint16(*((uint16_t*)ptr)) & 0x1FFF;
    ptr+=2;

    if(ptr+1 > end) return TSD_INVALID_DATA_SIZE;
    hdr->transport_scrambling_control = (*ptr) >> 6 & 0x03;
    hdr->adaptation_field_control = (*ptr) >> 4 & 0x03;
    hdr->continuity_counter = (*ptr) & 0x0F;
    ptr++;

    if(hdr->adaptation_field_control == AFC_ADAPTATION_FIELD_AND_PAYLOAD ||
       hdr->adaptation_field_control == AFC_ADAPTATION_FIELD_ONLY) {
        if(ptr+4 > end) return TSD_INVALID_DATA_SIZE;

        TSCode res = parse_adaptation_field(ctx, ptr, size-4,
                                            &hdr->adaptation_field);

        if(res != TSD_OK) return res;

        if(ptr+hdr->adaptation_field.adaptation_field_length > end) {
            return TSD_INVALID_DATA_SIZE;
        };
        ptr += hdr->adaptation_field.adaptation_field_length;
    }

    if(hdr->adaptation_field_control == AFC_NO_FIELD_PRESENT ||
       hdr->adaptation_field_control == AFC_ADAPTATION_FIELD_AND_PAYLOAD) {
        hdr->data_bytes = ptr;
        hdr->data_bytes_length = TSD_TSPACKET_SIZE - (((size_t)ptr) - ((size_t)data));
    } else {
        hdr->data_bytes = NULL;
        hdr->data_bytes_length = 0;
    }

    return TSD_OK;
}

TSCode parse_adaptation_field(TSDemuxContext *ctx,
                              const void *data,
                              size_t size,
                              AdaptationField *adap)
{
    if(ctx == NULL)                 return TSD_INVALID_CONTEXT;
    if(data == NULL)                return TSD_INVALID_DATA;
    if(size == 0)                   return TSD_INVALID_DATA_SIZE;
    if(adap == NULL)                return TSD_INVALID_ARGUMENT;

    const uint8_t *ptr = (const uint8_t *)data;
    const uint8_t *end = &ptr[size];

    if(ptr+1 > end) return TSD_INVALID_DATA_SIZE;

    adap->adaptation_field_length = *ptr;
    if(adap->adaptation_field_length > 0) {
        ptr++;
        if(ptr+1 > end) return TSD_INVALID_DATA_SIZE;
        adap->flags = *ptr;
        ptr++;

        if(adap->flags & AF_PCR_FLAG) {
            if(ptr+6 > end) return TSD_INVALID_DATA_SIZE;
            uint64_t val = parse_uint64(*((uint64_t*)ptr));
            adap->program_clock_reference_base = (val >> 31) & 0x1FFFFFFFFL;
            // ignore the 6 reserved bytes
            adap->program_clock_reference_extension = (uint16_t)((val >> 16) & 0x1FFL);
            ptr+=6;
        }

        if(adap->flags & AF_OPCR_FLAG) {
            if(ptr+6 > end) return TSD_INVALID_DATA_SIZE;
            uint64_t val = parse_uint64(*((uint64_t*)ptr));
            adap->original_program_clock_reference_base = (val >> 31) & 0x1FFFFFFFFL;
            // ignore the 6 reserved bytes
            adap->original_program_clock_reference_extension = (uint16_t)((val >> 16) & 0x1FFL);
            ptr+=6;
        }

        if(adap->flags & AF_SPLICING_POINT_FLAG) {
            if(ptr+1 > end) return TSD_INVALID_DATA_SIZE;
            adap->splice_countdown = *ptr;
            ptr++;
        }

        if(adap->flags & AF_TRANSPORT_PRIVATE_DATA_FLAG) {
            if(ptr+1 > end) return TSD_INVALID_DATA_SIZE;
            adap->transport_private_data_length = *ptr;
            ptr++;

            if(ptr + adap->transport_private_data_length > end) {
                return TSD_INVALID_DATA_SIZE;
            }
            adap->private_data_byte = ptr;
            ptr += adap->transport_private_data_length;
        }

        if(adap->flags & AF_ADAPTATION_FIELD_EXTENSION_FLAG) {
            if(ptr+1 > end) return TSD_INVALID_DATA_SIZE;
            adap->adaptation_field_extension.length = *ptr;
            ptr++;
            if(ptr+1 > end) return TSD_INVALID_DATA_SIZE;
            adap->adaptation_field_extension.flags = ((*ptr) >> 5) & 0x07;
            ptr++;

            if(adap->adaptation_field_extension.flags & AFEF_LTW_FLAG) {
                if(ptr+2 > end) return TSD_INVALID_DATA_SIZE;
                adap->adaptation_field_extension.ltw_valid_flag =
                    (*ptr) >> 7 & 0x01;
                uint16_t offset = parse_uint16(*((uint16_t*)ptr));
                adap->adaptation_field_extension.ltw_offset = offset & 0x7FFF;
                ptr += 2;
            }

            if(adap->adaptation_field_extension.flags & AFEF_PIECEWISE_RATE_FLAG) {
                if(ptr+3 > end) return TSD_INVALID_DATA_SIZE;
                uint32_t rate = parse_uint32(*((uint32_t*)ptr)) >> 8;
                adap->adaptation_field_extension.piecewise_rate = rate & 0x3FFFFF;
                ptr += 3;
            }

            if(adap->adaptation_field_extension.flags & AFEF_SEAMLESS_SPLCE_FLAG) {
                if(ptr+5 > end) return TSD_INVALID_DATA_SIZE;
                adap->adaptation_field_extension.splice_type = ((*ptr) >> 4) & 0x0F;
                uint32_t au1 = (uint64_t) (((*ptr) >> 1) & 0x07);
                uint32_t au2 = (uint64_t) ((parse_uint16(*((uint16_t*)(ptr+1))) >> 1) & 0x7FFF);
                uint32_t au3 = (uint64_t) ((parse_uint16(*((uint16_t*)(ptr+3))) >> 1) & 0x7FFF);
                adap->adaptation_field_extension.dts_next_au = (au1 << 30) | (au2 << 15) | au3;
            }
        }
    }

    return TSD_OK;
}

TSCode get_data_context(TSDemuxContext *ctx,
                        uint8_t table_id,
                        uint16_t table_ext,
                        uint8_t version,
                        DataContext **context)
{
    if(ctx == NULL)         return TSD_INVALID_CONTEXT;
    if(context == NULL)     return TSD_INVALID_ARGUMENT;

    // create a 32-bit Id for this table
    uint32_t id = (((uint32_t)table_id) << 24) |
                  (((uint32_t)table_ext) << 8) |
                  ((uint32_t)version);

    // go through the DataContexts already Available and see if we have a match
    size_t i;
    for(i=0; i<ctx->buffers.length; ++i) {
        if(ctx->buffers.pool[i].id == id) {
            // found a matching DataContext
            *context = &ctx->buffers.pool[i];
            return TSD_OK;
        }
    }

    // we didn't find a DataContext, create a new one and add it to the Pool
    DataContext *dataCtx = NULL;
    size_t len = ctx->buffers.length + 1;
    if(len == 1) {
        dataCtx = (DataContext*) ctx->malloc(sizeof(DataContext));
    } else {
        size_t size = sizeof(DataContext) * len;
        dataCtx = (DataContext*) ctx->realloc(dataCtx, size);
    }

    if(!dataCtx) {
        return TSD_OUT_OF_MEMORY;
    }
    ctx->buffers.pool = dataCtx;
    ctx->buffers.length = len;
    dataCtx = &dataCtx[len - 1];

    // initialize the DataContext
    TSCode res = data_context_init(ctx, dataCtx);
    *context = dataCtx;
    return res;
}

TSCode parse_table(TSDemuxContext *ctx,
                   TSPacket *pkt,
                   Table *table)
{
    if(ctx == NULL)                 return TSD_INVALID_CONTEXT;
    if(pkt == NULL)                 return TSD_INVALID_ARGUMENT;
    if(table == NULL)               return TSD_INVALID_ARGUMENT;

    TSCode res;

    // payload data error handling
    if(pkt->data_bytes == NULL || pkt->data_bytes_length == 0) {
        return TSD_INCOMPLETE_TABLE;
    }

    size_t pointer_field = 0;

    if(pkt->flags & TSPF_PAYLOAD_UNIT_START_INDICATOR) {
        // there is a new table section somewhere in this packet.
        // if we haven't starting writing any table sections yet we'll need to
        // jump to this location
        if(ctx->buffers.active == NULL) {
            pointer_field = ((size_t)pkt->data_bytes[0]) + 1;
        } else {
            pointer_field = 1;
        }

        // parse some of the table info so that we can find the DataContext
        // assoicated with this table
        const uint8_t *ptr = &pkt->data_bytes[pointer_field];
        uint8_t table_id = *ptr;
        uint16_t table_ext = 0;
        uint8_t version = 0;
        if(ptr[1] & 0x80) {
            table_ext = parse_uint16(*((uint16_t*)&ptr[3]));
            version = (ptr[5] & 0x3E) >> 1;
        }
        // set the DataContext for this table as active
        res = get_data_context(ctx,
                               table_id,
                               table_ext,
                               version,
                               &ctx->buffers.active);
        if(res != TSD_OK) {
            return res;
        }

    } else {
        // if we don't already have an active buffer this must be old table data
        // that we aren't prepared to parse
        if(ctx->buffers.active == NULL) {
            return TSD_INCOMPLETE_TABLE;
        }
    }

    DataContext *dataCtx = ctx->buffers.active;
    const uint8_t *write_pos = pkt->data_bytes + pointer_field;
    size_t write_size = pkt->data_bytes_length - pointer_field;

    // write the data into our buffer
    res = data_context_write(ctx, dataCtx, write_pos, write_size);
    if(res != TSD_OK) {
        return res;
    }

    // has the table completed yet? We'll need enough data to see the section
    // length to figure that out.
    uint8_t *ptr = dataCtx->buffer;
    int section_count = 0;

    while(ptr < dataCtx->write) {
        uint16_t section_len = parse_uint16(*((uint16_t*)(ptr+1)));
        section_len &= 0x0FFF;
        ptr += section_len + 3;
        section_count++;

        if((ptr+1) < dataCtx->write && *(ptr+1) == 0xFF) {
            // we found the end of the table, create and parse the sections
            table->length = section_count;
            table->sections = (TableSection*) ctx->calloc(section_count,
                              sizeof(TableSection));

            if(!table->sections) return TSD_OUT_OF_MEMORY;

            // parse the table sections
            res = parse_table_sections(ctx,
                                       dataCtx->buffer,
                                       dataCtx->write - dataCtx->buffer,
                                       table);
            return res;
        }
    }

    return TSD_INCOMPLETE_TABLE;
}

TSCode parse_table_sections(TSDemuxContext *ctx,
                            uint8_t *data,
                            size_t size,
                            Table *table)
{
    if(ctx == NULL)             return TSD_INVALID_CONTEXT;
    if(data == NULL)            return TSD_INVALID_DATA;
    if(size == 0)               return TSD_INVALID_DATA_SIZE;
    if(table == NULL)            return TSD_INVALID_ARGUMENT;

    uint8_t *ptr = data;
    size_t i;

    for(i=0; i < table->length; ++i) {
        TableSection *section = &(table->sections[i]);
        section->table_id = *ptr;
        ptr++;
        // section syntax indicator and private indicator
        section->flags = (int)(((*ptr) >> 6) & 0x03);
        section->section_length = parse_uint16(*((uint16_t*)ptr)) & 0x0FFF;
        ptr+=2;
        // are we dealing with a long form or short form table?
        if(section->flags & TBL_SECTION_SYNTAX_INDICATOR) {
            // long form table properties
            // Note that the pat_transport_stream_id is a union between PAT, CAT
            // and PMT data.
            section->u16.pat_transport_stream_id = parse_uint16(*((uint16_t*)(ptr)));
            section->version_number = ((*(ptr+2)) >> 1) & 0x1F;
            section->flags |= ((*(ptr+2)) & 0x01) << 2; // current next indicator
            section->section_number = *(ptr+3);
            section->last_section_number = *(ptr+4);
            section->section_data = ptr+5;
            uint32_t *ptr32 = (uint32_t*)(&ptr[section->section_length - 4]);
            section->crc_32 = parse_uint32(*ptr32);
            // the crc32 is not counted in the data
            section->section_data_length = section->section_length - 9;
        } else {
            // set everything to zero for short form
            section->u16.pat_transport_stream_id = 0;
            section->version_number = 0;
            section->section_number = 0;
            section->last_section_number = 0;
            section->crc_32 = 0;
            section->section_data = ptr;
            section->section_data_length = section->section_length;
        }

        ptr += section->section_length;
    }

    return TSD_OK;
}

TSCode parse_pat(TSDemuxContext *ctx,
                 const uint8_t *data,
                 size_t size,
                 PATData *pat)
{
    if(ctx == NULL)                 return TSD_INVALID_CONTEXT;
    if(data == NULL)                return TSD_INVALID_DATA;
    if(size < 4)                    return TSD_INVALID_DATA_SIZE;
    if(pat == NULL)                 return TSD_INVALID_ARGUMENT;

    size_t count = size / 4;
    size_t new_length = pat->length + count;
    uint16_t *pid_data = NULL;
    uint16_t *prog_data = NULL;

    if(pat->length) {
        pid_data = (uint16_t*)ctx->realloc(pat->pid, new_length * 2);
        prog_data = (uint16_t*)ctx->realloc(pat->program_number, new_length * 2);
    } else {
        pid_data = (uint16_t*)ctx->malloc(new_length * 2);
        prog_data = (uint16_t*)ctx->malloc(new_length * 2);
    }

    if(!pid_data || !prog_data) {
        if(prog_data) ctx->free(prog_data);
        if(pid_data) ctx->free(pid_data);
        return TSD_OUT_OF_MEMORY;
    }

    size_t i;
    for(i=pat->length; i < new_length; ++i) {
        prog_data[i] = parse_uint16(*((uint16_t*)data));
        data += 2;
        pid_data[i] = parse_uint16(*((uint16_t*)data)) & 0x1FFF;
        data += 2;
    }

    pat->pid = pid_data;
    pat->program_number = prog_data;
    pat->length = new_length;

    return TSD_OK;
}

size_t descriptor_count(const uint8_t *ptr, size_t length)
{
    // count the number of descriptors
    const uint8_t *end = &ptr[length];
    size_t count = 0;

    while(ptr < end) {
        ++count;
        uint8_t desc_len = ptr[1];
        ptr = &ptr[2 + desc_len];
    }

    return count;
}

size_t parse_descriptor(const uint8_t* data,
                        size_t size,
                        Descriptor *descriptors,
                        size_t length)
{
    size_t i=0;
    Descriptor *desc;
    const uint8_t *ptr = data;
    const uint8_t *end = &data[size];

    for(i=0; i < length && ptr < end; ++i) {
        desc = &descriptors[i];
        desc->tag = ptr[0];
        desc->length = ptr[1];
        desc->data = &ptr[2];
        ptr = &ptr[desc->length + 2];
    }

    return (size_t)(ptr - data);
}

TSCode parse_pmt(TSDemuxContext *ctx,
                 const uint8_t *data,
                 size_t size,
                 PMTData *pmt)
{
    if(ctx == NULL)                 return TSD_INVALID_CONTEXT;
    if(data == NULL)                return TSD_INVALID_DATA;
    if(size < 4)                    return TSD_INVALID_DATA_SIZE;
    if(pmt == NULL)                 return TSD_INVALID_ARGUMENT;

    const uint8_t *ptr = data;
    pmt->pcr_pid = parse_uint16(*((uint16_t*)ptr)) & 0x1FFF;
    ptr += 2;
    pmt->program_info_length = parse_uint16(*((uint16_t*)ptr)) & 0x0FFF;
    ptr += 2;

    // parse the outter descriptor into a one-dimensional array
    size_t desc_size = (size_t)pmt->program_info_length;
    size_t count = 0;

    if(desc_size > 0) {
        count = descriptor_count(ptr, desc_size);
        // create and parse the descriptors
        pmt->descriptors = (Descriptor*) ctx->calloc(count,
                           sizeof(Descriptor));
        if(!pmt->descriptors) return TSD_OUT_OF_MEMORY;
        pmt->descriptors_length = count;

        // parse all the outter descriptors
        parse_descriptor(ptr, desc_size, pmt->descriptors, count);
        ptr = &ptr[desc_size];
    }

    // parse the program elements.
    // As above, determine how many program elements we will have
    const uint8_t *pe_ptr = ptr;
    const uint8_t *pe_end = &data[size - 4]; // acounting for the CRC32
    count = 0; // reset the counter

    while(pe_ptr < pe_end) {
        ++count;
        pe_ptr += 3;
        uint16_t len = parse_uint16(*((uint16_t*)pe_ptr)) & 0x0FFF;
        pe_ptr = &pe_ptr[len];
    }

    // there might not be any Program Elements
    if(count == 0) {
        pmt->crc_32 = parse_uint32(*((uint32_t*)ptr));
        return TSD_OK;
    }

    pmt->program_elements = (ProgramElement*) ctx->calloc(count,
                            sizeof(ProgramElement));

    if(!pmt->program_elements) {
        ctx->free(pmt->descriptors);
        pmt->descriptors = NULL;
        pmt->descriptors_length = 0;
        return TSD_OUT_OF_MEMORY;
    }

    // parse the Program Elements
    pmt->program_elements_length = count;
    size_t i;
    for(i=0; i<count; ++i) {
        ProgramElement *prog = &pmt->program_elements[i];
        prog->stream_type = *ptr;
        ptr++;
        prog->elementary_pid = parse_uint16(*((uint16_t*)ptr)) & 0x1FFF;
        ptr += 2;
        prog->es_info_length = parse_uint16(*((uint16_t*)ptr)) & 0x0FFF;
        ptr += 2;
        // parse the inner descriptors for each program as above,
        // find out how many there are, then allocate a single array
        desc_size = (size_t) prog->es_info_length;
        if(desc_size == 0) {
            prog->descriptors = NULL;
            prog->descriptors_length = 0;
            continue;
        }

        size_t inner_count = descriptor_count(ptr, desc_size);
        if(inner_count == 0) {
            prog->descriptors = NULL;
            prog->descriptors_length = 0;
            ptr = &ptr[desc_size];
            continue;
        }

        prog->descriptors = (Descriptor*) ctx->calloc(inner_count,
                            sizeof(Descriptor));
        prog->descriptors_length = inner_count;
        parse_descriptor(ptr, desc_size, prog->descriptors, inner_count);
        ptr = &ptr[desc_size];
    }

    pmt->crc_32 = parse_uint32(*((uint32_t*)ptr));
    return TSD_OK;
}

TSCode parse_pes(TSDemuxContext *ctx,
                 const void *data,
                 size_t size,
                 PESPacket *pes)
{
    if(ctx == NULL)                 return TSD_INVALID_CONTEXT;
    if(data == NULL)                return TSD_INVALID_DATA;
    if(size < 6)                   return TSD_INVALID_DATA_SIZE;
    if(pes == NULL)                 return TSD_INVALID_ARGUMENT;

    const uint8_t *ptr = (uint8_t*)data;
    const uint8_t *end = &ptr[size];

    if(ptr+1 > end) return TSD_INVALID_DATA_SIZE;
    uint32_t value = parse_uint32(*((uint32_t*)ptr));
    if((value >> 8) != 0x01) return TSD_INVALID_START_CODE_PREFIX;

    pes->stream_id = (uint8_t)(value & 0x000000FF);

    return TSD_OK;
}

TSCode parse_cat(TSDemuxContext *ctx,
                 const uint8_t *data,
                 size_t size,
                 CATData* cat)
{

    if(ctx == NULL)                 return TSD_INVALID_CONTEXT;
    if(data == NULL)                return TSD_INVALID_DATA;
    if(size < 2)                    return TSD_INVALID_DATA_SIZE;
    if(cat == NULL)                 return TSD_INVALID_ARGUMENT;

    size_t count = descriptor_count(data, size);
    if(count == 0) {
        cat->descriptors = NULL;
        cat->descriptors_length = 0;
    } else {
        cat->descriptors = (Descriptor*) ctx->calloc(count, sizeof(Descriptor));
        if(!cat->descriptors) return TSD_OUT_OF_MEMORY;
        cat->descriptors_length = count;
    }

    return TSD_OK;
}

TSCode data_context_init(TSDemuxContext *ctx, DataContext *dataCtx)
{
    if(ctx == NULL)         return TSD_INVALID_CONTEXT;
    if(dataCtx == NULL)     return TSD_INVALID_ARGUMENT;

    dataCtx->buffer = (uint8_t*)ctx->malloc(TSD_DEFAULT_DATA_CONTEXT_SIZE);
    dataCtx->end = dataCtx->buffer + TSD_DEFAULT_DATA_CONTEXT_SIZE;
    dataCtx->write = dataCtx->buffer;
    dataCtx->size = TSD_DEFAULT_DATA_CONTEXT_SIZE;

    return TSD_OK;
}

TSCode data_context_destroy(TSDemuxContext *ctx, DataContext *dataCtx)
{
    if(ctx == NULL)         return TSD_INVALID_CONTEXT;
    if(dataCtx == NULL)     return TSD_INVALID_ARGUMENT;

    if(dataCtx->buffer != NULL) {
        ctx->free(dataCtx->buffer);
        memset(dataCtx, 0, sizeof(DataContext));
    }

    return TSD_OK;
}

TSCode data_context_write(TSDemuxContext *ctx, DataContext *dataCtx, const uint8_t *data, size_t size)
{
    if(ctx == NULL)         return TSD_INVALID_CONTEXT;
    if(dataCtx == NULL)     return TSD_INVALID_ARGUMENT;
    if(data == NULL)        return TSD_INVALID_DATA;
    if(size == 0)           return TSD_INVALID_DATA_SIZE;

    // if we don't have enough space we'll need to reallocate the  data.
    size_t space = (size_t)(dataCtx->end - dataCtx->write);
    if(space < size) {
        // reallocate enough memory aligning it to the default size
        size_t align = TSD_DEFAULT_DATA_CONTEXT_SIZE;
        size_t new_size = dataCtx->size + ((((size-space)/align) + 1) * align);
        size_t used = dataCtx->size - space;

        void *mem = ctx->realloc(dataCtx->buffer, new_size);
        if(!mem) {
            return TSD_OUT_OF_MEMORY;
        }

        dataCtx->buffer = (uint8_t*)mem;
        dataCtx->end = dataCtx->buffer + new_size;
        dataCtx->write = dataCtx->buffer + used;
        dataCtx->size = new_size;
    }

    // write the data into the buffer
    memcpy(dataCtx->write, data, size);
    dataCtx->write += size;
    return TSD_OK;
}

TSCode data_context_reset(TSDemuxContext *ctx, DataContext *dataCtx)
{
    if(ctx == NULL)         return TSD_INVALID_CONTEXT;
    if(dataCtx == NULL)     return TSD_INVALID_ARGUMENT;

    dataCtx->write = dataCtx->buffer;
    return TSD_OK;
}

TSCode extract_table_data(TSDemuxContext *ctx,
                          TSPacket *hdr,
                          Table *table,
                          uint8_t **mem,
                          size_t *size)
{
    memset(table, 0, sizeof(Table));
    TSCode res = parse_table(ctx, hdr, table);
    DataContext *data = ctx->buffers.active;

    if(res != TSD_OK && res != TSD_INCOMPLETE_TABLE) {
        data_context_reset(ctx, data);
        ctx->buffers.active = NULL;
        return res;
    }

    // the could be incomplete at this point, so continue parsing
    if(res == TSD_INCOMPLETE_TABLE || (data && data->size == 0)) {
        return TSD_INCOMPLETE_TABLE;
    }

    // we have a complete table.
    // create a contiguous memory buffer for parsing the table
    size_t block_size = data->write - data->buffer;
    void *block = ctx->malloc(block_size);
    if(!block) {
        data_context_reset(ctx, data);
        ctx->buffers.active = NULL;
        return TSD_OUT_OF_MEMORY;
    }

    uint8_t *ptr = (uint8_t*) block;
    size_t i;
    size_t written = 0;

    // go through all the sections and copy them into our buffer
    for(i=0; i<table->length; ++i) {
        TableSection *sec = &table->sections[i];
        if(!sec->section_data || sec->section_data_length == 0) {
            continue;
        }
        size_t len = sec->section_data_length;
        memcpy(ptr, sec->section_data, len);
        written += len;
        ptr = &ptr[len];
    }

    *size = written;
    *mem = block;

    return TSD_OK;
}

TSCode demux_pat(TSDemuxContext *ctx, TSPacket *hdr)
{
    uint8_t *block = NULL;
    size_t written = 0;
    Table table;
    TSCode res = extract_table_data(ctx,
                                    hdr,
                                    &table,
                                    &block,
                                    &written);
    if(res != TSD_OK) {
        return res;
    }

    // TODO: If the protocol version is not zero pass this table to the user
    //       instead of trying to parse it.
    // if(table.length > 0) {
    //     if(ctx->event_cb) {
    //         TableData tdata;
    //         tdata.table = &table;
    //         tdata.data = block;
    //         tdata.size = written;
    //         ctx->event_cb(ctx, TSD_EVENT_TABLE, (void*)&tdata);
    //         // cleanup
    //         ctx->free((void *)block);
    //     }
    //     return TSD_OK;
    // }

    // parse the PAT
    PATData *pat = &ctx->pat.value;
    memset(pat, 0, sizeof(PATData));
    res = parse_pat(ctx, block, written, pat);

    if(TSD_OK == res) {
        ctx->pat.valid = 1;
        // create PMT parsers for each of the Programs
        size_t pat_len = ctx->pat.value.length;
        if(ctx->pmt.capacity < pat_len) {
            // free the existing data
            if(ctx->pmt.values) {
                ctx->free(ctx->pmt.values);
            }
            // allocate new array
            if(pat_len > 0) {
                ctx->pmt.values = (PMTData*) ctx->calloc(pat_len, sizeof(PMTData));
                if(!ctx->pmt.values) {
                    ctx->free(block);
                    return TSD_OUT_OF_MEMORY;
                }
            }
            ctx->pmt.capacity = pat_len;
        }
        ctx->pmt.length = pat_len;

        // call the user callback
        if(ctx->event_cb) {
            ctx->event_cb(ctx, TSD_EVENT_PAT, (void*)pat);
        }
    } else {
        // we're not sure what went wrong... something royal
        ctx->pat.valid = 0;
        return TSD_PARSE_ERROR;
    }

    // cleanup
    ctx->free(block);

    return TSD_OK;
}

TSCode demux_pmt(TSDemuxContext *ctx, TSPacket *hdr, size_t pmt_idx)
{
    uint8_t *block = NULL;
    size_t written = 0;
    Table table;
    TSCode res = extract_table_data(ctx,
                                    hdr,
                                    &table,
                                    &block,
                                    &written);
    if(res != TSD_OK) {
        return res;
    }
    // parse the PMT Table
    res = parse_pmt(ctx, block, written, &ctx->pmt.values[pmt_idx]);
    if(TSD_OK == res) {
        if(ctx->event_cb) {
            ctx->event_cb(ctx, TSD_EVENT_PMT, (void*)&ctx->pmt.values[pmt_idx]);
        }
    }

    // cleanup
    ctx->free(block);

    return res;
}

TSCode demux_descriptors(TSDemuxContext *ctx, TSPacket *hdr)
{
    uint8_t *block = NULL;
    size_t written = 0;
    Table table;
    TSCode res = extract_table_data(ctx,
                                    hdr,
                                    &table,
                                    &block,
                                    &written);
    if(res != TSD_OK) {
        return res;
    }

    // TODO: call the callback with the descriptors and Table data

    return TSD_OK;
}

size_t demux(TSDemuxContext *ctx,
             void *data,
             size_t size,
             TSCode *code)
{
    if(ctx == NULL)         return TSD_INVALID_CONTEXT;
    if(data == NULL)        return TSD_INVALID_DATA;
    if(size == 0)           return TSD_INVALID_DATA_SIZE;

    uint8_t *ptr = (uint8_t*)data;
    size_t remaining = size;

    // initially set the code to OK
    if(code != NULL) {
        *code = TSD_OK;
    }

    TSPacket hdr;
    TSCode res;

    while(remaining >= TSD_TSPACKET_SIZE) {
        res = parse_packet_header(ctx, ptr, size, &hdr);
        if(res != TSD_OK) {
            // set the code to the error
            if(code != NULL) {
                *code = res;
            }
            break;
        }

        remaining -= TSD_TSPACKET_SIZE;
        ptr += TSD_TSPACKET_SIZE;

        // skip packets with errors and null packets
        if((hdr.flags & TSPF_TRANSPORT_ERROR_INDICATOR) ||
           (hdr.pid == PID_NULL_PACKETS)) {
            continue;
        }

        if(hdr.pid == PID_PAT) {
            res = demux_pat(ctx, &hdr);
            if(res != TSD_OK && res != TSD_INCOMPLETE_TABLE) {
                return res;
            }
        } else if(hdr.pid == PID_CAT || hdr.pid == PID_TSDT) {
            // TODO: res =
        } else if (hdr.pid > PID_RESERVED_NON_ATSC &&
                   hdr.pid <= PID_GENERAL_PURPOSE) {
            // check to see if this PID is a PMT
            if(ctx->pat.valid) {
                size_t len = ctx->pat.value.length;
                uint16_t *pids = ctx->pat.value.pid;
                size_t i;

                for(i=0; i<len; ++i) {
                    if(pids[i] == hdr.pid) {
                        res = demux_pmt(ctx, &hdr, i);
                        if(res == TSD_INCOMPLETE_TABLE) {
                            continue;
                        } else if(res != TSD_OK) {
                            return res;
                        }
                    }
                }
            }
        }
    }

    // cleaup all the buffers if there isn't an active one
    if(ctx->buffers.active == NULL) {
        size_t len = ctx->buffers.length;
        size_t i;
        for(i=0; i<len; ++i) {
            data_context_destroy(ctx, &ctx->buffers.pool[i]);
        }
        ctx->free(ctx->buffers.pool);
        ctx->buffers.pool = NULL;
        ctx->buffers.length = 0;
    }

    return size - remaining;
}
