/*
 * MIT License
 *
 * Copyright (c) 2017 Edward Freeman
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "tsdemux.h"
#include "string.h"
#include <stdio.h>

uint16_t parse_u16(const uint8_t *bytes)
{
    uint16_t val = *((uint16_t*)bytes);
#ifdef __BIG_ENDIAN__
    return val;
#else
    return ((val >> 8) & 0x00FF) | ((val << 8) & 0xFF00);
#endif
}

uint64_t parse_u32(const uint8_t *bytes)
{
    uint32_t val = *((uint32_t*)bytes);
#ifdef __BIG_ENDIAN__
    return val;
#else
    return ((val >> 24) & 0x000000FF) |
           ((val >>  8) & 0x0000FF00) |
           ((val <<  8) & 0x00FF0000) |
           ((val << 24) & 0xFF000000) ;
#endif
}

uint64_t parse_u64(const uint8_t *bytes)
{
    uint64_t val = *((uint64_t*)bytes);
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

const char* tsd_get_version(void)
{
    return TSD_VERSION;
}

TSDCode tsd_context_init(TSDemuxContext *ctx)
{
    if(ctx == NULL) return TSD_INVALID_CONTEXT;

    memset(ctx, 0, sizeof(TSDemuxContext));

    ctx->malloc = malloc;
    ctx->realloc = realloc;
    ctx->calloc = calloc;
    ctx->free = free;

    // initialize the user defined event callback
    ctx->event_cb = (tsd_on_event) NULL;

    return TSD_OK;
}

TSDCode tsd_context_destroy(TSDemuxContext *ctx)
{
    if(ctx == NULL) return TSD_INVALID_CONTEXT;

    int i=0;
    int size = ctx->registered_pids_length;
    // destroyregistered pid list
    for(i=0; i<size; ++i) {
        tsd_data_context_destroy(ctx, ctx->registered_pids_data[i]);
        ctx->free(ctx->registered_pids_data[i]);
    }

    // destroy data context buffer pool
    size = ctx->buffers.length;
    for(i=0; i<size; ++i) {
        tsd_data_context_destroy(ctx, &ctx->buffers.pool[i]);
    }
    if(size) {
        ctx->free(ctx->buffers.pool);
    }

    // destroy pmt value data
    if(ctx->pmt.capacity > 0) {
        ctx->free(ctx->pmt.values);
    }

    // destroy PAT data
    if(ctx->pat.valid && ctx->pat.value.length > 0) {
        ctx->free(ctx->pat.value.pid);
        ctx->free(ctx->pat.value.program_number);
    }

    // clear everything
    memset(ctx, 0, sizeof(TSDemuxContext));

    return TSD_OK;
}

TSDCode tsd_set_event_callback(TSDemuxContext *ctx, tsd_on_event callback)
{
    if(ctx == NULL)     return TSD_INVALID_CONTEXT;
    // Note that we allow the callback to be NULL
    ctx->event_cb = callback;
    return TSD_OK;
}

TSDCode tsd_parse_packet_header(TSDemuxContext *ctx,
                                const uint8_t *data,
                                size_t size,
                                TSDPacket *hdr)
{
    if(ctx == NULL)                 return TSD_INVALID_CONTEXT;
    if(data == NULL)                return TSD_INVALID_DATA;
    if(size < TSD_TSPACKET_SIZE)    return TSD_INVALID_DATA_SIZE;
    if(hdr == NULL)                 return TSD_INVALID_ARGUMENT;

    const uint8_t *ptr = data;
    const uint8_t *end = &ptr[size];
    // check the sync byte
    hdr->sync_byte = *ptr;
    if(hdr->sync_byte != TSD_SYNC_BYTE)  return TSD_INVALID_SYNC_BYTE;
    ptr++;

    hdr->flags = (*ptr) >> 5;
    hdr->pid = parse_u16(ptr) & 0x1FFF;
    ptr+=2;

    hdr->transport_scrambling_control = ((*ptr) >> 6) & 0x03;
    hdr->adaptation_field_control = ((*ptr) >> 4) & 0x03;
    hdr->continuity_counter = (*ptr) & 0x0F;
    ptr++;

    hdr->data_bytes = NULL;
    hdr->data_bytes_length = 0;

    // parse the adaptation field if it exists
    if(hdr->adaptation_field_control == TSD_AFC_ADAP_FIELD_AND_PAYLOAD ||
       hdr->adaptation_field_control == TSD_AFC_ADAP_FIELD_ONLY) {

        TSDCode res = tsd_parse_adaptation_field(ctx, ptr, size-4,
                      &hdr->adaptation_field);

        if(res != TSD_OK) return res;

        if(end < &ptr[hdr->adaptation_field.adaptation_field_length]) {
            return TSD_INVALID_DATA_SIZE;
        };
        ptr = &ptr[hdr->adaptation_field.adaptation_field_length + 1];
    } else {
        memset(&hdr->adaptation_field, 0, sizeof(TSDAdaptationField));
    }

    // is there a payload in this packet?
    if(hdr->adaptation_field_control == TSD_AFC_NO_FIELD_PRESENT ||
       hdr->adaptation_field_control == TSD_AFC_ADAP_FIELD_AND_PAYLOAD) {

        // handle error packets
        if(!(hdr->flags & TSD_PF_TRAN_ERR_INDICATOR) &&
           (hdr->pid != TSD_PID_NULL_PACKETS)) {

            size_t len = TSD_TSPACKET_SIZE - ((size_t)ptr - (size_t)data);
            if(len <= TSD_TSPACKET_SIZE - 4) {
                // note that there may or may not be a pointer_field in the
                // data, we can't tell whether there is yet.
                hdr->data_bytes = ptr;
                hdr->data_bytes_length = len;
            }
        }
    }

    return TSD_OK;
}

TSDCode tsd_parse_adaptation_field(TSDemuxContext *ctx,
                                   const uint8_t *data,
                                   size_t size,
                                   TSDAdaptationField *adap)
{
    if(ctx == NULL)                 return TSD_INVALID_CONTEXT;
    if(data == NULL)                return TSD_INVALID_DATA;
    if(size == 0)                   return TSD_INVALID_DATA_SIZE;
    if(adap == NULL)                return TSD_INVALID_ARGUMENT;

    const uint8_t *ptr = data;
    const uint8_t *end = &ptr[size];

    if(ptr+1 > end) return TSD_INVALID_DATA_SIZE;

    adap->adaptation_field_length = *ptr;
    if(adap->adaptation_field_length > TSD_TSPACKET_SIZE - 5) {
        return TSD_PARSE_ERROR;
    }

    if(adap->adaptation_field_length > 0) {
        ptr++;
        if(ptr+1 > end) return TSD_INVALID_DATA_SIZE;
        adap->flags = *ptr;
        ptr++;

        if(adap->flags & TSD_AF_PCR_FLAG) {
            if(ptr+6 > end) return TSD_INVALID_DATA_SIZE;
            uint64_t val = parse_u64(ptr);
            adap->program_clock_ref_base = (val >> 31) & 0x1FFFFFFFFL;
            // ignore the 6 reserved bytes
            adap->program_clock_ref_ext = (uint16_t)((val >> 16) & 0x1FFL);
            ptr+=6;
        }

        if(adap->flags & TSD_AF_OPCR_FLAG) {
            if(ptr+6 > end) return TSD_INVALID_DATA_SIZE;
            uint64_t val = parse_u64(ptr);
            adap->orig_program_clock_ref_base = (val >> 31) & 0x1FFFFFFFFL;
            // ignore the 6 reserved bytes
            adap->orig_program_clock_ref_ext = (uint16_t)((val >> 16) & 0x1FFL);
            ptr+=6;
        }

        if(adap->flags & TSD_AF_SPLICING_POINT_FLAG) {
            if(ptr+1 > end) return TSD_INVALID_DATA_SIZE;
            adap->splice_countdown = *ptr;
            ptr++;
        }

        if(adap->flags & TSD_AF_TRAN_PRIVATE_DATA_FLAG) {
            if(ptr+1 > end) return TSD_INVALID_DATA_SIZE;
            adap->transport_private_data_length = *ptr;
            ptr++;

            if(ptr + adap->transport_private_data_length > end) {
                return TSD_INVALID_DATA_SIZE;
            }
            adap->private_data_byte = ptr;
            ptr += adap->transport_private_data_length;
        }

        if(adap->flags & TSD_AF_ADAP_FIELD_EXT_FLAG) {
            if(ptr+1 > end) return TSD_INVALID_DATA_SIZE;
            adap->adap_field_ext.length = *ptr;
            ptr++;
            if(ptr+1 > end) return TSD_INVALID_DATA_SIZE;
            adap->adap_field_ext.flags = ((*ptr) >> 5) & 0x07;
            ptr++;

            if(adap->adap_field_ext.flags & TSD_AFEF_LTW_FLAG) {
                if(ptr+2 > end) return TSD_INVALID_DATA_SIZE;
                adap->adap_field_ext.ltw_valid_flag =
                    (*ptr) >> 7 & 0x01;
                uint16_t offset = parse_u16(ptr);
                adap->adap_field_ext.ltw_offset = offset & 0x7FFF;
                ptr += 2;
            }

            if(adap->adap_field_ext.flags & TSD_AFEF_PIECEWISE_RATE_FLAG) {
                if(ptr+3 > end) return TSD_INVALID_DATA_SIZE;
                uint32_t rate = parse_u32(ptr) >> 8;
                adap->adap_field_ext.piecewise_rate = rate & 0x3FFFFF;
                ptr += 3;
            }

            if(adap->adap_field_ext.flags & TSD_AFEF_SEAMLESS_SPLCE_FLAG) {
                if(ptr+5 > end) return TSD_INVALID_DATA_SIZE;
                adap->adap_field_ext.splice_type = ((*ptr) >> 4) & 0x0F;
                uint64_t au1 = (uint64_t) (((*ptr) >> 1) & 0x07);
                uint64_t au2 = (uint64_t) ((parse_u16(ptr+1) >> 1) & 0x7FFF);
                uint64_t au3 = (uint64_t) ((parse_u16(ptr+3) >> 1) & 0x7FFF);
                adap->adap_field_ext.dts_next_au = (au1 << 30) | (au2 << 15) | au3;
            }
        }
    }

    return TSD_OK;
}

TSDCode get_data_context(TSDemuxContext *ctx,
                         uint8_t table_id,
                         uint16_t table_ext,
                         uint8_t version,
                         TSDDataContext **context)
{
    if(ctx == NULL)         return TSD_INVALID_CONTEXT;
    if(context == NULL)     return TSD_INVALID_ARGUMENT;

    // create a 32-bit Id for this table
    uint32_t id = (((uint32_t)table_id) << 24) |
                  (((uint32_t)table_ext) << 8) |
                  ((uint32_t)version);

    // go through the DataContexts already Available and see if we have a match
    size_t i=0;
    for(; i<ctx->buffers.length; ++i) {
        if(ctx->buffers.pool[i].id == id) {
            // found a matching TSDDataContext
            *context = &ctx->buffers.pool[i];
            return TSD_OK;
        }
    }

    // we'll need to find the active buffer if one is set
    int active_idx = -1;
    if(ctx->buffers.active) {
        for(i=0; i<ctx->buffers.length; ++i) {
            if(ctx->buffers.pool[i].id == ctx->buffers.active->id) {
                active_idx = (int)i;
                break;
            }
        }
    }

    // we didn't find a TSDDataContext, create a new one and add it to the Pool
    TSDDataContext *dataCtx = NULL;
    size_t len = ctx->buffers.length + 1;
    if(len == 1) {
        dataCtx = (TSDDataContext*) ctx->malloc(sizeof(TSDDataContext));
    } else {
        size_t size = sizeof(TSDDataContext) * len;
        dataCtx = (TSDDataContext*) ctx->realloc(ctx->buffers.pool, size);
    }

    if(!dataCtx) {
        return TSD_OUT_OF_MEMORY;
    }
    ctx->buffers.pool = dataCtx;
    ctx->buffers.length = len;
    // reassign the active buffer
    if(active_idx >= 0) {
        ctx->buffers.active = &ctx->buffers.pool[active_idx];
    }

    // initialize the TSDDataContext
    TSDCode res = tsd_data_context_init(ctx, &(dataCtx[len-1]));
    *context = dataCtx;
    return res;
}

TSDCode tsd_parse_table(TSDemuxContext *ctx,
                        TSDPacket *pkt,
                        TSDTable *table)
{
    if(ctx == NULL)                 return TSD_INVALID_CONTEXT;
    if(pkt == NULL)                 return TSD_INVALID_ARGUMENT;
    if(table == NULL)               return TSD_INVALID_ARGUMENT;

    TSDCode res;

    // payload data error handling.
    if(pkt->data_bytes == NULL || pkt->data_bytes_length == 0) {
        return TSD_INCOMPLETE_TABLE;
    }

    // keep hold of the data location.
    const uint8_t *ptr = pkt->data_bytes;
    size_t ptr_len = pkt->data_bytes_length;

    if(pkt->flags & TSD_PF_PAYLOAD_UNIT_START_IND) {
        // there is a new table section somewhere in this packet.
        // parse the pointer_field.
        size_t pointer_field = *ptr;
        if(pointer_field >= pkt->data_bytes_length - 6) {
            return TSD_INVALID_POINTER_FIELD;
        }
        ptr = &ptr[pointer_field + 1];
        ptr_len -= (pointer_field + 1);
        // parse some of the table info so that we can find the TSDDataContext
        // assoicated with this table.
        uint8_t table_id = *ptr;
        uint16_t table_ext = 0;
        uint8_t version = 0;
        if(ptr[1] & 0x80) {
            table_ext = parse_u16(&ptr[3]);
            version = (ptr[5] & 0x3E) >> 1;
        }

        // set the TSDDataContext for this table as active.
        res = get_data_context(ctx,
                               table_id,
                               table_ext,
                               version,
                               &(ctx->buffers.active));
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

    TSDDataContext *dataCtx = ctx->buffers.active;
    // write the data into our buffer
    res = tsd_data_context_write(ctx, dataCtx, ptr, ptr_len);
    if(res != TSD_OK) {
        return res;
    }

    // has the table completed yet? We'll need enough data to see the section
    // length to figure that out.
    ptr = dataCtx->buffer;
    int section_count = 0;

    while(ptr < dataCtx->write) {
        uint16_t section_len = parse_u16(ptr+1);
        section_len &= 0x0FFF;
        ptr += section_len + 3;
        section_count++;

        if((ptr+1) < dataCtx->write && *(ptr+1) == 0xFF) {
            // we found the end of the table,
            // clear the active buffer seeing as we've finished with the table
            ctx->buffers.active = NULL;

            // create and parse the sections.
            table->length = section_count;
            table->sections = (TSDTableSection*) ctx->calloc(section_count,
                              sizeof(TSDTableSection));

            if(!table->sections) return TSD_OUT_OF_MEMORY;

            // parse the table sections
            res = tsd_parse_table_sections(ctx,
                                           dataCtx->buffer,
                                           dataCtx->write - dataCtx->buffer,
                                           table);

            if(res != TSD_OK) {
                tsd_table_data_destroy(ctx, table);
            }
            return res;
        }
    }

    return TSD_INCOMPLETE_TABLE;
}

TSDCode tsd_parse_table_sections(TSDemuxContext *ctx,
                                 uint8_t *data,
                                 size_t size,
                                 TSDTable *table)
{
    if(ctx == NULL)             return TSD_INVALID_CONTEXT;
    if(data == NULL)            return TSD_INVALID_DATA;
    if(size == 0)               return TSD_INVALID_DATA_SIZE;
    if(table == NULL)            return TSD_INVALID_ARGUMENT;

    uint8_t *ptr = data;
    uint8_t *end = &data[size];
    size_t i;

    for(i=0; i < table->length; ++i) {
        // make sure we have enough data to parse the table info
        if((size_t)(end - ptr) < 3) {
            return TSD_INVALID_DATA_SIZE;
        }
        TSDTableSection *section = &(table->sections[i]);
        section->table_id = *ptr;
        ptr++;
        // section syntax indicator and private indicator
        section->flags = (int)(((*ptr) >> 6) & 0x03);
        section->section_length = parse_u16(ptr) & 0x0FFF;
        ptr+=2;
        if(&ptr[section->section_length] > end) {
            return TSD_INVALID_DATA_SIZE;
        }
        // are we dealing with a long form or short form table?
        if(section->flags & TSD_TBL_SECTION_SYNTAX_INDICATOR) {
            // long form table properties
            // Note that the table_id_extension could be PAT, CAT or PMT data.
            // make sure we have enough data, and that the section length is
            // long enought to parse the table info.
            if((size_t)(end - ptr) < 9 || section->section_length < 9) {
                return TSD_INVALID_DATA_SIZE;
            }
            section->table_id_extension = parse_u16(ptr);
            section->version_number = ((*(ptr+2)) >> 1) & 0x1F;
            // current next indicator
            section->flags |= ((*(ptr+2)) & 0x01) << 2;
            section->section_number = *(ptr+3);
            section->last_section_number = *(ptr+4);
            section->section_data = ptr+5;
            section->crc_32 = parse_u32(&ptr[section->section_length - 4]);
            // the crc32 is not counted in the data
            section->section_data_length = section->section_length - 9;
        } else {
            // set everything to zero for short form
            section->table_id_extension = 0;
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

TSDCode tsd_parse_pat(TSDemuxContext *ctx,
                      const uint8_t *data,
                      size_t size,
                      TSDPATData *pat)
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
        pid_data = (uint16_t*)ctx->realloc(pat->pid,
                                           new_length * sizeof(uint16_t));
        prog_data = (uint16_t*)ctx->realloc(pat->program_number,
                                            new_length * sizeof(uint16_t));
    } else {
        pid_data = (uint16_t*)ctx->malloc(new_length * sizeof(uint16_t));
        prog_data = (uint16_t*)ctx->malloc(new_length *sizeof(uint16_t));
    }

    if(!pid_data || !prog_data) {
        if(prog_data) ctx->free(prog_data);
        if(pid_data) ctx->free(pid_data);
        return TSD_OUT_OF_MEMORY;
    }

    size_t i;
    for(i=pat->length; i < new_length; ++i) {
        prog_data[i] = parse_u16(data);
        data += 2;
        pid_data[i] = parse_u16(data) & 0x1FFF;
        data += 2;
    }

    pat->pid = pid_data;
    pat->program_number = prog_data;
    pat->length = new_length;

    return TSD_OK;
}

TSDCode destroy_pat_data(TSDemuxContext *ctx, TSDPATData *pat)
{
    if(ctx == NULL)     return TSD_INVALID_CONTEXT;
    if(pat == NULL)     return TSD_INVALID_ARGUMENT;

    if(pat->length > 0) {
        ctx->free(pat->program_number);
        ctx->free(pat->pid);
    }
    pat->length = 0;
    pat->program_number = pat->pid = NULL;
    return TSD_OK;
}

size_t descriptor_count(const uint8_t *ptr, size_t length)
{
    // count the number of descriptors
    const uint8_t *end = &ptr[length];
    size_t count = 0;

    while((ptr+2) < end) {
        ++count;
        uint8_t desc_len = ptr[1];
        if(desc_len > 0 && &ptr[2+desc_len] <= end) {
            ptr = &ptr[2 + desc_len];
        } else {
            break;
        }
    }

    return count;
}

size_t parse_descriptor(const uint8_t* data,
                        size_t size,
                        TSDDescriptor *descriptors,
                        size_t length)
{
    TSDDescriptor *desc;
    const uint8_t *ptr = data;
    const uint8_t *end = &data[size];

    size_t i=0;
    for(; i < length && (ptr+2) < end; ++i) {
        desc = &descriptors[i];
        desc->tag = ptr[0];
        desc->length = ptr[1];
        // make sure we have enough data to correctly parse the descriptors
        if(desc->length > 0 && &ptr[desc->length + 2] <= end) {
            desc->data = &ptr[2];
            ptr = &ptr[desc->length + 2];
        } else {
            desc->data = NULL;
            desc->length = 0;
            ptr = end;
        }
    }

    return (size_t)(ptr - data);
}

TSDCode destroy_pmt_data(TSDemuxContext *ctx, TSDPMTData *pmt)
{
    if(ctx == NULL)     return TSD_INVALID_CONTEXT;
    if(pmt == NULL)     return TSD_INVALID_ARGUMENT;

    if(pmt->descriptors_length > 0) {
        ctx->free(pmt->descriptors);
    }
    if(pmt->program_elements_length > 0) {
        int size = pmt->program_elements_length;
        int i = 0;
        for(; i<size; ++i) {
            TSDProgramElement *prog = &pmt->program_elements[i];
            if(prog != NULL && prog->descriptors_length > 0) {
                free(prog->descriptors);
            }
        }
        ctx->free(pmt->program_elements);
    }

    memset(pmt, 0, sizeof(TSDPMTData));
    return TSD_OK;
}

TSDCode tsd_parse_pmt(TSDemuxContext *ctx,
                      const uint8_t *data,
                      size_t size,
                      TSDPMTData *pmt)
{
    if(ctx == NULL)                 return TSD_INVALID_CONTEXT;
    if(data == NULL)                return TSD_INVALID_DATA;
    if(size < 4)                    return TSD_INVALID_DATA_SIZE;
    if(pmt == NULL)                 return TSD_INVALID_ARGUMENT;

    const uint8_t *ptr = data;
    const uint8_t *end = &ptr[size];

    pmt->pcr_pid = parse_u16(ptr) & 0x1FFF;
    ptr += 2;
    pmt->program_info_length = parse_u16(ptr) & 0x0FFF;
    ptr += 2;

    size_t desc_size = (size_t)pmt->program_info_length;
    // make sure we have enough data to parse the desciptors
    if(&ptr[desc_size] > end) {
        return TSD_INVALID_DATA_SIZE;
    }

    // parse the outter descriptor into a one-dimensional array
    size_t count = 0;
    if(desc_size > 0) {
        count = descriptor_count(ptr, desc_size);
        // create and parse the descriptors
        pmt->descriptors = (TSDDescriptor*) ctx->calloc(count,
                           sizeof(TSDDescriptor));
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
        uint16_t len = parse_u16(pe_ptr) & 0x0FFF;
        pe_ptr = &pe_ptr[len];
    }

    // there might not be any Program Elements
    if(count == 0) {
        pmt->crc_32 = parse_u32(ptr);
        ctx->free(pmt->descriptors);
        pmt->descriptors = NULL;
        pmt->descriptors_length = 0;
        return TSD_OK;
    }

    pmt->program_elements = (TSDProgramElement*) ctx->calloc(count,
                            sizeof(TSDProgramElement));

    if(!pmt->program_elements) {
        ctx->free(pmt->descriptors);
        pmt->descriptors = NULL;
        pmt->descriptors_length = 0;
        return TSD_OUT_OF_MEMORY;
    }

    // parse the Program Elements
    pmt->program_elements_length = count;
    size_t i;
    TSDCode res = TSD_OK;
    for(i=0; i<count; ++i) {
        TSDProgramElement *prog = &pmt->program_elements[i];
        prog->stream_type = *ptr;
        ptr++;
        prog->elementary_pid = parse_u16(ptr) & 0x1FFF;
        ptr += 2;
        prog->es_info_length = parse_u16(ptr) & 0x0FFF;
        ptr += 2;
        // parse the inner descriptors for each program as above,
        // find out how many there are, then allocate a single array
        desc_size = (size_t) prog->es_info_length;
        if(desc_size == 0) {
            prog->descriptors = NULL;
            prog->descriptors_length = 0;
            continue;
        }

        // make sure we make enough data to parse the descriptors
        if(&ptr[desc_size] > end) {
            res = TSD_INVALID_DATA_SIZE;
            break;
        }

        size_t inner_count = descriptor_count(ptr, desc_size);
        if(inner_count == 0) {
            prog->descriptors = NULL;
            prog->descriptors_length = 0;
            ptr = &ptr[desc_size];
            continue;
        }

        prog->descriptors = (TSDDescriptor*) ctx->calloc(inner_count,
                            sizeof(TSDDescriptor));
        prog->descriptors_length = inner_count;
        parse_descriptor(ptr, desc_size, prog->descriptors, inner_count);
        ptr = &ptr[desc_size];
    }

    if(res != TSD_OK)  {
        destroy_pmt_data(ctx, pmt);
        return res;
    }

    pmt->crc_32 = parse_u32(ptr);

    return TSD_OK;
}

TSDCode tsd_parse_pes(TSDemuxContext *ctx,
                      const uint8_t *data,
                      size_t size,
                      TSDPESPacket *pes)
{
    if(ctx == NULL)                 return TSD_INVALID_CONTEXT;
    if(data == NULL)                return TSD_INVALID_DATA;
    if(size < 6)                    return TSD_INVALID_DATA_SIZE;
    if(pes == NULL)                 return TSD_INVALID_ARGUMENT;

    memset(pes, 0, sizeof(TSDPESPacket));

    const uint8_t *ptr = data;
    const uint8_t *end = &ptr[size];

    uint32_t value = parse_u32(ptr);
    pes->start_code = (value >> 8);
    if(pes->start_code != 0x01) return TSD_INVALID_START_CODE_PREFIX;

    pes->stream_id = (uint8_t)(value & 0x000000FF);
    ptr = &ptr[4];
    pes->packet_length = parse_u16(ptr);
    ptr = &ptr[2];

    if(pes->stream_id == TSD_PSID_PADDING_STREAM) {
        // Padding, we don't need to do anything
    } else if(pes->stream_id == TSD_PSID_PROGRAM_STREAM_MAP ||
              pes->stream_id == TSD_PSID_PRIV_STREAM_2 ||
              pes->stream_id == TSD_PSID_ECM ||
              pes->stream_id == TSD_PSID_EMM ||
              pes->stream_id == TSD_PSID_STREAM_DIRECTORY ||
              pes->stream_id == TSD_PSID_DSMCC ||
              pes->stream_id == TSD_PSID_H2221_TYPE_E) {
        pes->data_bytes = ptr;
    } else {
        uint8_t value = *ptr;
        ptr++;
        pes->scrambling_control = (value & 0x30) >> 6;
        pes->flags = ((value & 0x0F) << 8) | *ptr;
        ptr++;
        pes->header_data_length = *ptr;
        ptr++;
        if(pes->flags & TSD_PPF_PTS_FLAG) {
            uint64_t pts1 = (uint64_t) (((*ptr) >> 1) & 0x07);
            uint64_t pts2 = (uint64_t) ((parse_u16(ptr+1) >> 1) & 0x7FFF);
            uint64_t pts3 = (uint64_t) ((parse_u16(ptr+3) >> 1) & 0x7FFF);
            pes->pts = (pts1 << 30) | (pts2 << 15) | pts3;
            ptr = &ptr[5];
        }
        if(pes->flags & TSD_PPF_DTS_FLAG) {
            uint64_t dts1 = (uint64_t) (((*ptr) >> 1) & 0x07);
            uint64_t dts2 = (uint64_t) ((parse_u16(ptr+1) >> 1) & 0x7FFF);
            uint64_t dts3 = (uint64_t) ((parse_u16(ptr+3) >> 1) & 0x7FFF);
            pes->dts = (dts1 << 30) | (dts2 << 15) | dts3;
            ptr = &ptr[5];
        }
        if(pes->flags & TSD_PPF_ESCR_FLAG) {
            uint64_t value = parse_u64(ptr);
            pes->escr = ((value >> 27) & 0x7FFFLL) |
                        (((value >> 43) & 0x7FFFLL) << 15) |
                        (((value >> 59) & 0x0007LL) << 30);
            pes->escr_extension = (uint16_t)((value >> 17) & 0x01FFLL);
            ptr = &ptr[6];
        }
        if(pes->flags & TSD_PPF_ES_RATE_FLAG) {
            uint32_t value = parse_u32(ptr);
            pes->es_rate = (value >> 9) & 0x003FFFFF;
            ptr = &ptr[3];
        }
        if(pes->flags & TSD_PPF_DSM_TRICK_MODE_FLAG) {
            pes->trick_mode.control = ((*ptr) >> 5) & 0x07;
            if(pes->trick_mode.control == TSD_TMC_FAST_FORWARD ||
               pes->trick_mode.control == TSD_TMC_FAST_REVERSE) {
                pes->trick_mode.field_id = ((*ptr) >> 3) & 0x03;
                pes->trick_mode.intra_slice_refresh = ((*ptr) >> 2) & 0x01;
                pes->trick_mode.frequency_truncation = (*ptr) & 0x03;
            } else if(pes->trick_mode.control == TSD_TMC_SLOW_MOTION ||
                      pes->trick_mode.control == TSD_TMC_SLOW_REVERSE) {
                pes->trick_mode.rep_cntrl = (*ptr) & 0x1F;
            } else if(pes->trick_mode.control == TSD_TMC_FREEZE_FRAME) {
                pes->trick_mode.field_id = ((*ptr) >> 3) & 0x03;
            }
            ptr++;
        }
        if(pes->flags & TSD_PPF_ADDITIONAL_COPY_INFO_FLAG) {
            pes->additional_copy_info = (*ptr) & 0x7F;
            ptr++;
        }
        if(pes->flags & TSD_PPF_PES_CRC_FLAG) {
            pes->previous_pes_packet_crc = parse_u16(ptr);
            ptr = &ptr[2];
        }
        if(pes->flags & TSD_PPF_PES_EXTENSION_FLAG) {
            pes->extension.flags = *ptr;
            ptr++;
            if(pes->extension.flags & TSD_PEF_PES_PRIVATE_DATA_FLAG) {
                int i=0;
                for(i=0; i<16; ++i) {
                    pes->extension.pes_private_data[i] = ptr[i];
                }
                ptr = &ptr[16]; // 128 bits
            }
            if(pes->extension.flags & TSD_PEF_PACK_HEADER_FIELD_FLAG) {
                TSDPackHeader *pheader = &pes->extension.pack_header;
                pheader->length = *ptr;
                ptr++;
                pheader->start_code = parse_u32(ptr);
                ptr = &ptr[4];
                uint64_t value = parse_u64(ptr);
                pheader->system_clock_ref_base =
                    ((value >> 27) & 0x7FFFLL) |
                    (((value >> 43) & 0x7FFFLL) << 15) |
                    (((value >> 59) & 0x0007LL) << 30);
                pheader->system_clock_ref_ext =
                    (uint16_t)((value >> 17) & 0x01FFLL);
                ptr = &ptr[6];
                pheader->program_mux_rate = parse_u32(ptr) >> 10;
                ptr = &ptr[3];
                pheader->stuffing_length = (*ptr) & 0x07;
                // get past stuffing
                ptr = &ptr[1 + pheader->stuffing_length];
                // System Header
                TSDSystemHeader *sysh = &pheader->system_header;
                sysh->start_code = parse_u32(ptr);
                ptr = &ptr[4];
                sysh->length = parse_u16(ptr);
                ptr = &ptr[2];
                sysh->rate_bound = (parse_u32(ptr) >> 9) & 0x003FFFFF;
                ptr = &ptr[3];
                sysh->audio_bound = (*ptr) >> 2;
                sysh->flags = parse_u32(ptr) & 0x03C08000;
                ptr++;
                sysh->video_bound = (*ptr) & 0x1F;
                ptr = &ptr[2];
                sysh->stream_count = (sysh->length - 6) / 3;
                if(sysh->stream_count > 0) {
                    sysh->streams = (TSDSystemHeaderStream*) ctx->calloc(sysh->stream_count, sizeof(TSDSystemHeaderStream));
                    if(!sysh->streams) return TSD_OUT_OF_MEMORY;
                    size_t i;
                    size_t used = 0;
                    for(i=0; i<sysh->stream_count; ++i) {
                        // make sure the first bit is set in the stream id
                        if((*ptr) & 0x80) {
                            used++;
                            TSDSystemHeaderStream *stream = &sysh->streams[i];
                            stream->stream_id = *ptr;
                            ptr++;
                            stream->pstd_buffer_bound_scale = ((*ptr) >> 5) & 0x01;
                            stream->pstd_buffer_size_bound = parse_u16(ptr) & 0x1FFF;
                            ptr = &ptr[2];
                        }
                    }
                    sysh->stream_count = used;
                }
            }
            if(pes->extension.flags & TSD_PEF_PROGRAM_PACKET_SEQUENCE_COUNTER_FLAG) {
                pes->extension.program_packet_sequence_counter = (*ptr) & 0x7F;
                ptr++;
                pes->extension.mpeg1_mpeg2_identifier = ((*ptr) >> 6) & 0x01;
                pes->extension.original_stuff_length = (*ptr) & 0x3F;
                ptr++;
            }
            if(pes->extension.flags & TSD_PEF_PSTD_BUFFER_FLAG) {
                pes->extension.pstd_buffer_scale = ((*ptr) >> 5) & 0x01;
                pes->extension.pstd_buffer_size = parse_u16(ptr) & 0x1FFF;
                ptr = &ptr[2];
            }
            if(pes->extension.flags & TSD_PEF_PES_EXTENSION_FLAG_2) {
                pes->extension.pes_extension_field_length = (*ptr) & 0x7F;
                ptr = &ptr[1 + pes->extension.pes_extension_field_length];
            }
        }
        // get past any stuffing
        while(*ptr == 0xFF) {
            ptr++;
        }
        pes->data_bytes = ptr;
        pes->data_bytes_length = end - ptr;
    }

    return TSD_OK;
}

TSDCode tsd_parse_descriptors(TSDemuxContext *ctx,
                              const uint8_t *data,
                              size_t size,
                              TSDDescriptorData *descriptorData)
{

    if(ctx == NULL)                 return TSD_INVALID_CONTEXT;
    if(data == NULL)                return TSD_INVALID_DATA;
    if(size < 2)                    return TSD_INVALID_DATA_SIZE;
    if(descriptorData == NULL)      return TSD_INVALID_ARGUMENT;

    size_t count = descriptor_count(data, size);
    if(count == 0) {
        descriptorData->descriptors = NULL;
        descriptorData->descriptors_length = 0;
    } else {
        descriptorData->descriptors = (TSDDescriptor*) ctx->calloc(count,
                                      sizeof(TSDDescriptor));
        if(!descriptorData->descriptors) return TSD_OUT_OF_MEMORY;
        descriptorData->descriptors_length = count;
    }

    return TSD_OK;
}

TSDCode tsd_data_context_init(TSDemuxContext *ctx, TSDDataContext *dataCtx)
{
    if(ctx == NULL)         return TSD_INVALID_CONTEXT;
    if(dataCtx == NULL)     return TSD_INVALID_ARGUMENT;

    dataCtx->buffer = (uint8_t*)ctx->malloc(TSD_MEM_PAGE_SIZE);
    dataCtx->end = dataCtx->buffer + TSD_MEM_PAGE_SIZE;
    dataCtx->write = dataCtx->buffer;
    dataCtx->size = TSD_MEM_PAGE_SIZE;

    return TSD_OK;
}

TSDCode tsd_data_context_destroy(TSDemuxContext *ctx, TSDDataContext *dataCtx)
{
    if(ctx == NULL)         return TSD_INVALID_CONTEXT;
    if(dataCtx == NULL)     return TSD_INVALID_ARGUMENT;

    if(dataCtx->buffer != NULL) {
        ctx->free(dataCtx->buffer);
        memset(dataCtx, 0, sizeof(TSDDataContext));
    }

    return TSD_OK;
}

TSDCode tsd_data_context_write(TSDemuxContext *ctx,
                               TSDDataContext *dataCtx,
                               const uint8_t *data,
                               size_t size)
{
    if(ctx == NULL)         return TSD_INVALID_CONTEXT;
    if(dataCtx == NULL)     return TSD_INVALID_ARGUMENT;
    if(data == NULL)        return TSD_INVALID_DATA;
    if(size == 0)           return TSD_INVALID_DATA_SIZE;

    // if we don't have enough space we'll need to reallocate the  data.
    size_t space = (size_t)(dataCtx->end - dataCtx->write);
    if(space < size) {
        // reallocate enough memory aligning it to the default size
        size_t align = TSD_MEM_PAGE_SIZE;
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

TSDCode tsd_data_context_reset(TSDemuxContext *ctx, TSDDataContext *dataCtx)
{
    if(ctx == NULL)         return TSD_INVALID_CONTEXT;
    if(dataCtx == NULL)     return TSD_INVALID_ARGUMENT;

    dataCtx->write = dataCtx->buffer;
    return TSD_OK;
}

TSDCode tsd_table_data_extract(TSDemuxContext *ctx,
                               TSDPacket *hdr,
                               TSDTable *table,
                               uint8_t **mem,
                               size_t *size)
{
    memset(table, 0, sizeof(TSDTable));
    TSDCode res = tsd_parse_table(ctx, hdr, table);

    if(res != TSD_OK && res != TSD_INCOMPLETE_TABLE) {
        // clear the active buffer if it is set
        tsd_data_context_reset(ctx, ctx->buffers.active);
        ctx->buffers.active = NULL;
        return res;
    }

    // there may not be any sections available yet.
    if(!table->sections) {
        return res;
    }

    // get the data context for the table in question using the first section
    // as a reference.
    TSDTableSection *section = &table->sections[0];
    TSDDataContext *data = NULL;
    res = get_data_context(ctx,
                           section->table_id,
                           section->table_id_extension,
                           section->version_number,
                           &data);

    // the table could be incomplete at this point, so continue parsing
    if(res == TSD_INCOMPLETE_TABLE || (data && data->size == 0)) {
        return TSD_INCOMPLETE_TABLE;
    }

    // we have a complete table.
    // create a contiguous memory buffer for parsing the table
    if(res != TSD_OK) {
        return res;
    }

    size_t block_size = data->write - data->buffer;
    if(block_size == 0) {
        return TSD_INVALID_DATA_SIZE;
    }

    void *block = ctx->malloc(block_size);
    if(!block) {
        tsd_data_context_reset(ctx, data);
        ctx->buffers.active = NULL;
        return TSD_OUT_OF_MEMORY;
    }

    uint8_t *ptr = (uint8_t*) block;
    uint8_t *end = &ptr[block_size];
    size_t i=0;
    size_t written = 0;

    // go through all the sections and copy them into our buffer
    for(; i<table->length; ++i) {
        TSDTableSection *sec = &table->sections[i];
        if(!sec->section_data || sec->section_data_length == 0) {
            continue;
        }
        size_t len = sec->section_data_length;
        // make sure we have enough room to accomodate the copy
        if(&ptr[len] > end) {
            ctx->free(block);
            return TSD_INVALID_DATA_SIZE;
        }
        memcpy(ptr, sec->section_data, len);
        written += len;
        ptr = &ptr[len];
    }

    *size = written;
    *mem = block;

    // reset the block of data
    tsd_data_context_reset(ctx, data);

    return TSD_OK;
}

TSDCode tsd_table_data_destroy(TSDemuxContext *ctx, TSDTable *table)
{
    if(ctx == NULL)     return TSD_INVALID_CONTEXT;
    if(table == NULL)   return TSD_INVALID_ARGUMENT;

    if(table->length) {
        ctx->free(table->sections);
    }

    table->length = 0;
    table->sections = NULL;

    return TSD_OK;
}

TSDCode demux_pat(TSDemuxContext *ctx, TSDPacket *hdr)
{
    uint8_t *block = NULL;
    size_t written = 0;
    TSDTable table;
    TSDCode res = tsd_table_data_extract(ctx,
                                         hdr,
                                         &table,
                                         &block,
                                         &written);
    if(res != TSD_OK) {
        return res;
    }

    // parse the PAT.
    // cleanup the old PAT data.
    if(ctx->pat.valid == 1) {
        ctx->pat.valid = 0;
        destroy_pat_data(ctx, &ctx->pat.value);
    }
    // parse the new PAT data.
    TSDPATData *pat = &ctx->pat.value;
    memset(pat, 0, sizeof(TSDPATData));
    res = tsd_parse_pat(ctx, block, written, pat);

    if(TSD_OK == res) {
        ctx->pat.valid = 1;
        // create PMT parsers for each of the Programs
        size_t pat_len = ctx->pat.value.length;
        if(ctx->pmt.capacity < pat_len) {
            // free the existing data
            if(ctx->pmt.values) {
                ctx->free(ctx->pmt.values);
                ctx->pmt.values = NULL;
            }
            // allocate new array
            if(pat_len > 0) {
                ctx->pmt.values = (TSDPMTData*) ctx->calloc(pat_len, sizeof(TSDPMTData));
                if(!ctx->pmt.values) {
                    ctx->free(block);
                    tsd_table_data_destroy(ctx, &table);
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
        tsd_table_data_destroy(ctx, &table);
        return TSD_PARSE_ERROR;
    }

    // cleanup
    ctx->free(block);
    tsd_table_data_destroy(ctx, &table);

    return TSD_OK;
}

TSDCode demux_pmt(TSDemuxContext *ctx, TSDPacket *hdr, size_t pmt_idx)
{
    uint8_t *block = NULL;
    size_t written = 0;
    TSDTable table;
    TSDCode res = tsd_table_data_extract(ctx,
                                         hdr,
                                         &table,
                                         &block,
                                         &written);
    if(res != TSD_OK) {
        return res;
    }
    // parse the PMT TSDTable
    //TSDPMTData *pmt = &ctx->pmt.values[pmt_idx];
    TSDPMTData pmt;
    memset(&pmt, 0, sizeof(pmt));
    res = tsd_parse_pmt(ctx, block, written, &pmt);

    if(TSD_OK == res) {
        if(ctx->event_cb) {
            ctx->event_cb(ctx, TSD_EVENT_PMT, (void*)&pmt);
        }
        // cleanup
        destroy_pmt_data(ctx, &pmt);
    }

    // cleanup
    ctx->free(block);
    tsd_table_data_destroy(ctx, &table);

    return res;
}

TSDCode demux_descriptors(TSDemuxContext *ctx, TSDPacket *hdr)
{
    uint8_t *block = NULL;
    size_t written = 0;
    TSDTable table;
    TSDCode res = tsd_table_data_extract(ctx,
                                         hdr,
                                         &table,
                                         &block,
                                         &written);
    if(res != TSD_OK) {
        return res;
    }

    // parse all the outter descriptors
    TSDDescriptorData descriptorData;
    res = tsd_parse_descriptors(ctx, block, written, &descriptorData);

    // call the callback with the descriptors and TSDTable data
    if(TSD_OK == res) {
        if(ctx->event_cb) {
            TSDEventId event;
            switch(hdr->pid) {
            case TSD_PID_CAT:
                event = TSD_EVENT_CAT;
                break;
            case TSD_PID_TSDT:
                event = TSD_EVENT_TSDT;
                break;
            default:
                event = TSD_EVENT_TSDT;
                break;
            }
            ctx->event_cb(ctx, event, (void*)&descriptorData);
        }
    }

    // cleanup
    tsd_table_data_destroy(ctx, &table);

    return TSD_OK;
}

size_t tsd_demux(TSDemuxContext *ctx,
                 void *data,
                 size_t size,
                 TSDCode *code)
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

    TSDPacket hdr;
    TSDCode res;

    while(remaining >= TSD_TSPACKET_SIZE) {
        res = tsd_parse_packet_header(ctx, ptr, size, &hdr);
        // if we run into an error try parsing the next packet.
        if(res != TSD_OK) {
            // if the error is due to an invalid sync byte, see if we can
            // lock onto a valid one.
            if(res == TSD_INVALID_SYNC_BYTE) {
                while(remaining >= TSD_TSPACKET_SIZE) {
                    remaining--;
                    ptr++;
                    // if we find a value for a sync byte try and use that.
                    if(*ptr == TSD_SYNC_BYTE) {
                        break;
                    }
                }
                continue;
            } else {
                // skip this packet
                remaining -= TSD_TSPACKET_SIZE;
                ptr += TSD_TSPACKET_SIZE;
                continue;
            }
        }

        remaining -= TSD_TSPACKET_SIZE;
        ptr += TSD_TSPACKET_SIZE;

        // skip packets with errors and null packets
        if((hdr.flags & TSD_PF_TRAN_ERR_INDICATOR) ||
           (hdr.pid == TSD_PID_NULL_PACKETS) ||
           (hdr.adaptation_field_control == TSD_AFC_RESERVED)) {
            continue;
        }

        // DEBUG: do we already have this PID saved
        size_t i;
        int found = 0;
        for(i=0; i<ctx->pids_length; ++i) {
            if(ctx->pids[i] == hdr.pid) {
                found = 1;
                break;
            }
        }
        // save the PID
        if(found == 0 && ctx->pids_length < 1024) {
            ctx->pids[ctx->pids_length] = hdr.pid;
            ctx->pids_length++;
        }

        if(hdr.pid == TSD_PID_PAT) {
            res = demux_pat(ctx, &hdr);
            if(res != TSD_OK && res != TSD_INCOMPLETE_TABLE) {
                return res;
            }
        } else if(hdr.pid == TSD_PID_CAT || hdr.pid == TSD_PID_TSDT) {
            res = demux_descriptors(ctx, &hdr);
            if(res != TSD_OK && res != TSD_INCOMPLETE_TABLE) {
                return res;
            }
        } else if (hdr.pid > TSD_PID_RESERVED_NON_ATSC &&
                   hdr.pid <= TSD_PID_GENERAL_PURPOSE) {
            // check to see if this PID is a PMT
            int parsed = 0;
            if(ctx->pat.valid) {
                size_t len = ctx->pat.value.length;
                uint16_t *pids = ctx->pat.value.pid;
                size_t i;

                for(i=0; i<len; ++i) {
                    if(pids[i] == hdr.pid) {
                        if(parsed != 1) parsed = 1;
                        res = demux_pmt(ctx, &hdr, i);
                        if(res == TSD_INCOMPLETE_TABLE) {
                            continue;
                        } else if(res != TSD_OK) {
                            return res;
                        }
                    }
                }
            }

            // if this isn't a PMT PID, check to usee if the user has registerd it
            if(parsed == 0) {
                if(hdr.flags & TSD_PF_PAYLOAD_UNIT_START_IND) {
                    size_t i;
                    for(i=0; i < ctx->registered_pids_length; ++i) {
                        if(ctx->registered_pids[i] != hdr.pid || !hdr.data_bytes) {
                            continue;
                        }
                        const uint8_t *ptr = hdr.data_bytes;
                        TSDDataContext *dataCtx = ctx->registered_pids_data[i];
                        size_t ptr_len = hdr.data_bytes_length;
                        if(hdr.flags & TSD_PF_PAYLOAD_UNIT_START_IND) {
                            uint8_t pointer_field = *ptr;
                            if(pointer_field >= ptr_len) {
                                continue;
                            }
                            ptr = &ptr[pointer_field+1];
                            ptr_len -= (pointer_field + 1);
                            tsd_data_context_reset(ctx, dataCtx);
                        }
                        tsd_data_context_write(ctx, dataCtx, ptr, ptr_len);

                        // get the PES length to see if we have the complete
                        // packet
                        size_t data_len = dataCtx->write - dataCtx->buffer;
                        if(data_len > 5) {
                            uint16_t pes_len = parse_u16(&dataCtx->buffer[5]) + 5;
                            if(data_len >= pes_len) {
                                TSDPESPacket pes;
                                res = tsd_parse_pes(ctx, dataCtx->buffer, data_len, &pes);
                                if(res != TSD_OK) {
                                    printf("bad PES parse\n");
                                } else {
                                    ctx->event_cb(ctx, TSD_EVENT_PID, (void *)&pes);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // cleaup all the buffers if there isn't an active one
    if(ctx->buffers.active == NULL && ctx->buffers.length > 0) {
        size_t len = ctx->buffers.length;
        size_t i;
        for(i=0; i<len; ++i) {
            tsd_data_context_destroy(ctx, &ctx->buffers.pool[i]);
        }
        ctx->free(ctx->buffers.pool);
        ctx->buffers.pool = NULL;
        ctx->buffers.length = 0;
    }

    return size - remaining;
}

TSDCode tsd_register_pid(TSDemuxContext *ctx, uint16_t pid)
{
    if(ctx == NULL)     return TSD_INVALID_CONTEXT;

    // make sure the pid isn't already registered
    size_t i;
    for(i=0; i<ctx->registered_pids_length; ++i) {
        if(ctx->registered_pids[i] == pid) {
            return TSD_PID_ALREADY_REGISTERED;
        }
    }

    // make sure we have room to register a new pid
    if(ctx->registered_pids_length == TSD_MAX_PID_REGS) {
        return TSD_TSD_MAX_PID_REGS_REACHED;
    }

    // register the new pid
    ctx->registered_pids[ctx->registered_pids_length] = pid;
    TSDDataContext *dataContext = (TSDDataContext*) ctx->malloc(sizeof(TSDDataContext));
    if(dataContext == NULL) {
        return TSD_OUT_OF_MEMORY;
    }

    TSDCode res = tsd_data_context_init(ctx, dataContext);
    if(res != TSD_OK) {
        ctx->free(dataContext);
        return res;
    }
    ctx->registered_pids_data[ctx->registered_pids_length] = dataContext;
    ctx->registered_pids_length++;

    return TSD_OK;
}

TSDCode tsd_deregister_pid(TSDemuxContext *ctx, uint16_t pid)
{
    if(ctx == NULL)     return TSD_INVALID_CONTEXT;

    // find the pid in the registration list
    size_t i;
    for(i=0; i<ctx->registered_pids_length; ++i) {
        if(ctx->registered_pids[i] == pid) {
            // remove this pid by shifting the pids in front of it down
            size_t j;
            for(j=i+1; j<ctx->registered_pids_length; ++j) {
                ctx->registered_pids[j-1] = ctx->registered_pids[j];
            }
            ctx->registered_pids_length--;
            return TSD_OK;
        }
    }
    return TSD_PID_NOT_FOUND;
}
