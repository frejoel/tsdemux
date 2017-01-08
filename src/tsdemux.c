#include "tsdemux.h"

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
    if(*ptr != TSD_SYNC_BYTE)  return TSD_INVALID_SYNC_BYTE;
    ptr++;
    // parse the flags (3)
    hdr->flags = (*ptr) & 0x07;
    // parse the 13 bit PID
    hdr->pid = ((*( (uint16_t *)ptr)) >> 3) & (uint16_t)(0x1FFF);
    ptr+=2;
    hdr->transport_scrambling_control = (*ptr) & 0x02;
    hdr->adaptation_field_control = ((*ptr) & 0x0C) >> 2;
    hdr->continuity_counter = ((*ptr) & 0xF0) >> 4;
    ptr++;

    if(hdr->adaptation_field_control == AFC_ADAPTATION_FIELD_AND_PAYLOAD ||
        hdr->adaptation_field_control == AFC_ADAPTATION_FIELD_ONLY)
    {
        // TODO: parse adapatation field
        //ptr += adaptation_len;
    }

    if(hdr->adaptation_field_control == AFC_NO_FIELD_PRESENT ||
        hdr->adaptation_field_control == AFC_ADAPTATION_FIELD_AND_PAYLOAD)
    {
        hdr->data_bytes = (void *)ptr;
    }else{
        hdr->data_bytes = NULL;
    }

    return TSD_OK;
}

TSCode parse_adaptation_field(TSDemuxContext *ctx,
                              const TSPacket *pkt,
                              AdaptationField *adap)
{
    return TSD_OK;
}
