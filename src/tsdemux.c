#include "tsdemux.h"

uint16_t parse_uint16(uint16_t val) {
#ifdef __BIG_ENDIAN__
    return val;
#else
    return (val >> 8 & 0x00FF) | (val << 8 & 0xFF00);
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
        hdr->adaptation_field_control == AFC_ADAPTATION_FIELD_ONLY)
    {
        TSCode res = parse_adaptation_field(ctx, ptr, size-4,
            &hdr->adaptation_field);

        if(res != TSD_OK) return res;

        ptr += hdr->adaptation_field.adaptation_field_length;
    }

    if(hdr->adaptation_field_control == AFC_NO_FIELD_PRESENT ||
        hdr->adaptation_field_control == AFC_ADAPTATION_FIELD_AND_PAYLOAD)
    {
        hdr->data_bytes = ptr;
    }else{
        hdr->data_bytes = NULL;
    }

    return TSD_OK;
}

TSCode parse_adaptation_field(TSDemuxContext *ctx,
                              const void *data,
                              size_t size,
                              AdaptationField *adap)
{
    return TSD_OK;
}
