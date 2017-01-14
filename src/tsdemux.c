#include "tsdemux.h"

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

    const uint8_t* ptr = (const uint8_t *)data;
    // check the sync byte
    hdr->sync_byte = *ptr;
    if(hdr->sync_byte != TSD_SYNC_BYTE)  return TSD_INVALID_SYNC_BYTE;
    ptr++;
    // parse the flags (3)
    hdr->flags = (*ptr) >> 5;
    hdr->pid = parse_uint16(*((uint16_t*)ptr)) & 0x1FFF;
    ptr+=2;
    hdr->transport_scrambling_control = (*ptr) >> 6 & 0x03;
    hdr->adaptation_field_control = (*ptr) >> 4 & 0x03;
    hdr->continuity_counter = (*ptr) & 0x0F;
    ptr++;

    if(hdr->adaptation_field_control == AFC_ADAPTATION_FIELD_AND_PAYLOAD ||
       hdr->adaptation_field_control == AFC_ADAPTATION_FIELD_ONLY) {
        TSCode res = parse_adaptation_field(ctx, ptr, size-4,
                                            &hdr->adaptation_field);

        if(res != TSD_OK) return res;

        ptr += hdr->adaptation_field.adaptation_field_length;
    }

    if(hdr->adaptation_field_control == AFC_NO_FIELD_PRESENT ||
       hdr->adaptation_field_control == AFC_ADAPTATION_FIELD_AND_PAYLOAD) {
        hdr->data_bytes = ptr;
    } else {
        hdr->data_bytes = NULL;
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

    const uint8_t* ptr = (const uint8_t *)data;

    adap->adaptation_field_length = *ptr;
    if(adap->adaptation_field_length > 0) {
        ptr++;
        adap->flags = *ptr;
        ptr++;

        if(adap->flags & AF_PCR_FLAG) {
            uint64_t val = parse_uint64(*((uint64_t*)ptr));
            adap->program_clock_reference_base = (val >> 31) & 0x1FFFFFFFFL;
            // ignore the 6 reserved bytes
            adap->program_clock_reference_extension = (uint16_t)((val >> 16) & 0x1FFL);
            ptr+=6;
        }

        if(adap->flags & AF_OPCR_FLAG) {
            uint64_t val = parse_uint64(*((uint64_t*)ptr));
            adap->original_program_clock_reference_base = (val >> 31) & 0x1FFFFFFFFL;
            // ignore the 6 reserved bytes
            adap->original_program_clock_reference_extension = (uint16_t)((val >> 16) & 0x1FFL);
            ptr+=6;
        }

        if(adap->flags & AF_SPLICING_POINT_FLAG) {
            adap->splice_countdown = *ptr;
            ptr++;
        }

        if(adap->flags & AF_TRANSPORT_PRIVATE_DATA_FLAG) {
            adap->transport_private_data_length = *ptr;
            ptr++;

            adap->private_data_byte = ptr;
            ptr += adap->transport_private_data_length;
        }

        if(adap->flags & AF_ADAPTATION_FIELD_EXTENSION_FLAG) {
            adap->adaptation_field_extension.length = *ptr;
            ptr++;
            adap->adaptation_field_extension.flags = ((*ptr) >> 5) & 0x07;
            ptr++;

            if(adap->adaptation_field_extension.flags & AFEF_LTW_FLAG) {
                adap->adaptation_field_extension.ltw_valid_flag =
                    (*ptr) >> 7 & 0x01;
                uint16_t offset = parse_uint16(*((uint16_t*)ptr));
                adap->adaptation_field_extension.ltw_offset = offset & 0x7FFF;
                ptr += 2;
            }

            if(adap->adaptation_field_extension.flags & AFEF_PIECEWISE_RATE_FLAG) {
                uint32_t rate = parse_uint32(*((uint32_t*)ptr)) >> 8;
                adap->adaptation_field_extension.piecewise_rate = rate & 0x3FFFFF;
                ptr += 3;
            }

            if(adap->adaptation_field_extension.flags & AFEF_SEAMLESS_SPLCE_FLAG) {
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
