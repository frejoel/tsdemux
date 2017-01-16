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

    ctx->malloc = malloc;
    ctx->realloc = realloc;
    ctx->calloc = calloc;
    ctx->free = free;

    ctx->on_table_cb = NULL;

    TSCode res = data_context_init(ctx, &ctx->pat.data);

    return res;
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

TSCode parse_table(TSDemuxContext *ctx,
                   DataContext *dataCtx,
                   TSPacket *pkt,
                   Table *table)
{
    if(ctx == NULL)                 return TSD_INVALID_CONTEXT;
    if(dataCtx == NULL)             return TSD_INVALID_ARGUMENT;
    if(pkt == NULL)                 return TSD_INVALID_ARGUMENT;
    if(table == NULL)               return TSD_INVALID_ARGUMENT;

    TSCode res;

    // if we find a PAT PID write the data into the buffer
    if(pkt->pid != PID_PAT) {
        return TSD_NOT_A_TABLE_PACKET;
    }

    // payload data error handling
    if(pkt->data_bytes == NULL || pkt->data_bytes_length == 0) {
        return TSD_INCOMPLETE_TABLE;
    }

    // If we've not written any data into the DataContext yet, make sure
    // we start with a packet that includes the start of a new section.
    // This section might not be the first one, but we'll worry about that
    // later.
    if((dataCtx->write == dataCtx->buffer) &&
       !(pkt->flags & TSPF_PAYLOAD_UNIT_START_INDICATOR)) {
        return TSD_INCOMPLETE_TABLE;
    }

    // write the data into our buffer
    res = data_context_write(ctx,
                             dataCtx,
                             pkt->data_bytes,
                             pkt->data_bytes_length);

    if(res != TSD_OK) {
        return res;
    }

    // do we have enough data to parse the next section?
    size_t data_len = dataCtx->write - dataCtx->buffer;
    if(data_len < 8) {
        return TSD_INCOMPLETE_TABLE;
    }

    // get the length of this section
    uint16_t section_len = parse_uint16( *((uint16_t*)(dataCtx->buffer+1)));
    section_len &= 0x0FFF;

    if(data_len < section_len + 3) {
        return TSD_INCOMPLETE_TABLE;
    }

    // we have enough data to parse this section, make sure it's a PAT table,
    // (we don't care about short vs long table format at this point)
    if(dataCtx->buffer[0] != 0x00) {
        return TSD_INCOMPLETE_TABLE;
    }
    // parse the PAT section
    TableSection *section = (TableSection*) ctx->malloc(
                                sizeof(TableSection));

    res = parse_table_section(ctx, dataCtx->buffer,
                              data_len,
                              section);

    if(res != TSD_OK) {
        return res;
    }

    // add the section to our Table
    res = add_table_section(ctx, table, section);
    if(res != TSD_OK) {
        return res;
    }

    return TSD_OK;
}

TSCode parse_table_section(TSDemuxContext *ctx,
                           const void *data,
                           size_t size,
                           TableSection *section)
{
    if(ctx == NULL)             return TSD_INVALID_CONTEXT;
    if(data == NULL)            return TSD_INVALID_DATA;
    if(size < 96)               return TSD_INVALID_DATA_SIZE;
    if(section == NULL)         return TSD_INVALID_ARGUMENT;

    const uint8_t *ptr = (uint8_t*)data;

    section->table_id = *ptr;
    ptr++;
    // section syntax indicator and private indicator
    section->flags = ((*ptr) >> 6) & 0x03;
    section->section_length = parse_uint16(*((uint16_t*)(ptr))) & 0x0FFF;
    ptr+=2;

    // are we dealing with a long form or short form table?
    size_t crc_byte_count = 4;
    if(section->flags & TBL_SECTION_SYNTAX_INDICATOR) {
        // long form table properties
        section->transport_stream_id = parse_uint16(*((uint16_t*)(ptr)));
        ptr+=2;
        section->version_number = ((*ptr) >> 1) & 0x1F;
        section->flags |= ((*ptr) & 0x01) << 2; // current next indicator
        ptr++;
        section->section_number = *ptr;
        ptr++;
        section->last_section_number = *ptr;
    } else {
        // set everything to zero for short form
        section->transport_stream_id = 0;
        section->version_number = 0;
        section->section_number = 0;
        section->last_section_number = 0;
        section->crc_32 = 0; // may or may not be populated later
        crc_byte_count = 0;
    }

    printf("section number: %d, last_section_length: %d\n", section->section_number, section->last_section_number);

    // PIDs in this section.
    // find the end of the PID sections
    uint8_t *pid_end = ((uint8_t*)data) + (size - crc_byte_count);
    // figure out how many PIDs are in this section (there's 4 bytes per pid)
    size_t pid_count = (pid_end - ptr) / 4;
    // allocate enough memory for all those PIDs
    //section->pids = (PIDMap*) ctx->calloc(pid_count, sizeof(PIDMap));

    // parse all the PIDs
    size_t i = 0;
    for(; i < pid_count; ++i) {
        //section->pids[i].program_number = parse_uint16(*((uint16_t*)ptr));
        //ptr+=2;
        //section->pids[i].pid = parse_uint16(*((uint16_t*)ptr)) & 0x1FFF;
        //ptr+=2;
        //printf("i: %d, pid: %x, prog number: %x\n", i, section->pids[i].pid, section->pids[i].program_number);
    }

    section->crc_32 = parse_uint32(*((uint32_t*)ptr));

    return TSD_OK;
}


TSCode add_table_section(TSDemuxContext *ctx, Table *table, TableSection *section)
{
    // allocate some space for the sections if we haven't already
    if(table->sections == NULL) {
        table->sections = (TableSection**) ctx->malloc(sizeof(size_t) * 16);

        if(table->sections == NULL) {
            return TSD_OUT_OF_MEMORY;
        }

        table->length = 0;
        table->capacity = 16;
    }

    if(table->length >= table->capacity) {
        size_t new_capacity = table->capacity + 16;
        TableSection **new_secs = (TableSection**) ctx->realloc((void*) table->sections,
                                  (sizeof(size_t) * new_capacity));

        if(new_secs == NULL) {
            return TSD_OUT_OF_MEMORY;
        }

        table->sections = new_secs;
        table->capacity = new_capacity;
    }

    table->sections[table->length] = section;
    table->length++;

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

size_t demux(TSDemuxContext *ctx,
             void *data,
             size_t size,
             TSCode *code)
{
    if(data == NULL)        return TSD_INVALID_DATA;
    if(size == 0)           return TSD_INVALID_DATA_SIZE;

    uint8_t *ptr = (uint8_t*)data;
    size_t remaining = size;

    // initially set the code to OK
    *code = TSD_OK;

    TSPacket hdr;
    TSCode res;

    while(remaining >= TSD_TSPACKET_SIZE) {
        res = parse_packet_header(ctx, ptr, size, &hdr);
        if(res != TSD_OK) {
            // set the code to the error
            *code = res;
            break;
        }

        remaining -= TSD_TSPACKET_SIZE;

        // skip packets with errors and null packets
        if((hdr.flags & TSPF_TRANSPORT_ERROR_INDICATOR) ||
           (hdr.pid == PID_NULL_PACKETS)) {
            continue;
        }

        if(hdr.pid == PID_PAT) {
            // parse the PAT
            res = parse_table(ctx, &ctx->pat.data, &hdr, &ctx->pat.table);
        } else if (hdr.pid == PID_CAT || hdr.pid == PID_TSDT ||
                   (hdr.pid > PID_RESERVED_FUTURE && hdr.pid <= PID_ATSC_PSIP_SI)) {
            // TODO:
        }
    }

    data_context_destroy(ctx, &ctx->pat.data);

    return size - remaining;
}
