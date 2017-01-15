#include "tsdemux.h"
#include "string.h"

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

TSCode parse_pat(TSDemuxContext *ctx,
                 DataContext *dataCtx,
                 const void *data,
                 size_t size,
                 PATable *pat)
{
    if(ctx == NULL)                 return TSD_INVALID_CONTEXT;
    if(dataCtx == NULL)             return TSD_INVALID_ARGUMENT;
    if(data == NULL)                return TSD_INVALID_DATA;
    if(size < TSD_TSPACKET_SIZE)    return TSD_INVALID_DATA_SIZE;
    if(pat == NULL)                 return TSD_INVALID_ARGUMENT;

    const void *ptr = data;
    size_t remaining = size;
    TSCode res;
    TSPacket pkt;

    // allocate some memory to store the raw PAT data into a buffer.
    // this is due to PAT data potentially being across multiple TS packets.
    if(dataCtx->buffer == NULL) {
        dataCtx->size = 256; // initial data buffer size
        dataCtx->buffer = (uint8_t*) ctx->malloc(dataCtx->size);
        dataCtx->write = dataCtx->buffer; // current write position
        dataCtx->end = dataCtx->buffer + dataCtx->size; // end of data
    }

    // start parsing packets to find the tables
    while(remaining >= TSD_TSPACKET_SIZE) {

        res = parse_packet_header(ctx, ptr, remaining, &pkt);
        if(res != TSD_OK) return res;
        remaining -= TSD_TSPACKET_SIZE;

        // skip error packets
        if(pkt.flags & TSPF_TRANSPORT_ERROR_INDICATOR) {
            continue;
        }

        // if we find a PAT PID write the data into our buffer
        if(pkt.pid == PID_PAT) {
            // payload data error handling
            if(pkt.data_bytes == NULL || pkt.data_bytes_length == 0) {
                continue;
            }

            // If we've not written any data yet, make sure we start with a
            // packet that includes the start of a new section.
            // This section might not be the first one however, but we'll
            // worry about that later.
            if((dataCtx->write == dataCtx->buffer) &&
               !(pkt.flags & TSPF_PAYLOAD_UNIT_START_INDICATOR)) {
                continue;
            }

            // if we don't have enough space we'll need to reallocate the  data.
            size_t space = (size_t)(dataCtx->end - dataCtx->write);
            if(space < pkt.data_bytes_length) {
                // reallocate enough memory aligning it to 256 bytes
                size_t new_size = dataCtx->size +
                                  (((pkt.data_bytes_length / 256) + 1) * 256);

                void *mem = ctx->realloc(dataCtx->buffer, new_size);
                if(!mem) {
                    return TSD_OUT_OF_MEMORY;
                }

                dataCtx->buffer = (uint8_t*)mem;
                dataCtx->end = &dataCtx->buffer[new_size];
                dataCtx->write = &dataCtx->buffer[dataCtx->size];
                dataCtx->size = new_size;
            }

            // write the data into our buffer
            memcpy(dataCtx->write, pkt.data_bytes, pkt.data_bytes_length);
            dataCtx->write += pkt.data_bytes_length;

            // do we have enough data to parse the next section?
            size_t data_len = dataCtx->write - dataCtx->buffer;
            if(data_len >= 8) {
                // get the length of this section
                uint16_t section_len = parse_uint16(
                                           *((uint16_t*)(dataCtx->buffer+1)));

                section_len &= 0x0FFF;
                if(data_len >= section_len + 3) {
                    // we have enough data to parse this section.
                    // make sure it's a PAT table, with the section syntax and
                    // '0' bit set correctly.
                    if((dataCtx->buffer[0] == 0x00) &&
                       ((dataCtx->buffer[1] & 0xC0) == 0x80)) {
                        // parse the PAT section
                        PATSection *section = (PATSection*) ctx->malloc(
                                                  sizeof(PATSection));

                        TSCode res;
                        res = parse_pat_section(ctx, data, data_len, section);
                        if(res != TSD_OK) return res;

                        res = add_pat_section(ctx, pat, section);
                        if(res != TSD_OK) return res;
                    }

                    // reset the write buffer
                    dataCtx->write = dataCtx->buffer;
                }
            }
        }
    }

    return TSD_OK;
}

TSCode parse_pat_section(TSDemuxContext *ctx,
                         const void *data,
                         size_t size,
                         PATSection *section)
{
    return TSD_OK;
}


TSCode add_pat_section(TSDemuxContext *ctx, PATable *pat, PATSection *section)
{
    // allocate some space for the sections if we haven't already
    if(pat->sections == NULL) {
        pat->sections = (PATSection**) ctx->malloc(sizeof(size_t) * 16);

        if(pat->sections == NULL) {
            return TSD_OUT_OF_MEMORY;
        }

        pat->length = 0;
        pat->capacity = 16;
    }

    if(pat->length >= pat->capacity) {
        size_t new_capacity = pat->capacity + 16;
        PATSection **new_secs = (PATSection**) ctx->realloc((void*) pat->sections,
                                (sizeof(size_t) * new_capacity));

        if(new_secs == NULL) {
            return TSD_OUT_OF_MEMORY;
        }

        pat->sections = new_secs;
        pat->capacity = new_capacity;
    }

    pat->sections[pat->length] = section;
    pat->length++;

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
