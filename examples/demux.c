/**
 * This examples demonstrates demuxing a data stream from the Transport-Stream.
 */

#include <stdio.h>
#include <string.h>

#include <tsdemux.h>

// demux callback (see tsd_set_event_callback for details).
void event_cb(TSDemuxContext *ctx, TSDEventId event_id, void *data);
// prints information about a PAT.
void print_pat(TSDemuxContext *ctx, void *data);
// prints information about a PMT.
void print_pmt(TSDemuxContext *ctx, void *data);


int main(int argc, char **charv) {
    // create a demuxing context.
    TSDemuxContext ctx;
    // set default values onto the context.
    tsd_set_default_context(&ctx);

    // add a callback.
    // the callback is used to determine which PIDs contain the data we want
    // to demux. We also receive PES data for any PIDs that we register later on.
    tsd_set_event_callback(&ctx, event_cb);

    // we'll be a using a TS file from disk.
    //FILE *fp = fopen("test/data/media_0_0.ts", "rb");
    FILE *fp = fopen("test/data/friends.ts", "rb");
    if(!fp) {
        fprintf(stderr, "Couldn't open test file\n");
        return 1;
    }
    // create a buffer on the stack which we'll use to read the file data into.
    // we'll set the buffer size to exactly 10 TS packets but it could be any
    // size.
    char buffer[1880];

    int count = 0; // number of bytes read from the file.
    size_t parsed = 0; // number of bytes parsed by the demuxer.

    // read the file until we reach the end.
    do {
        // at the end of the do loop, we write back any remainder bytes into
        // the buffer, this is why we write into the buffer at the location
        // &buffer[count - parsed].
        count = fread(&buffer[count - parsed], 1, 1880 - (count - parsed), fp);

        // with res, we could report any errors found during demuxing
        TSDCode res;
        parsed = tsd_demux(&ctx, buffer, count, &res);
        // during 'demux' our callback may be called, so we can safely discard
        // our buffer.
        // we'll copy any unused bytes back into the start of the buffer.
        // this may happen if we read partial packets from file, or there was
        // some corruption in the TS stream.
        if(parsed < count) {
            memcpy(buffer, buffer, count - parsed);
        }
    } while(count > 0);

    // close the file
    fclose(fp);

    return 0;
}

void event_cb(TSDemuxContext *ctx, TSDEventId event_id, void *data)
{
    if(event_id == TSD_EVENT_PAT) {
        print_pat(ctx, data);
    }else if(event_id == TSD_EVENT_PMT){
        print_pmt(ctx, data);
    }else if(event_id == TSD_EVENT_PID) {
        TSDPESPacket *pes = (TSDPESPacket*) data;
        // TODO: This is where we would write the PES data into our buffer
        //printf("\n====================\n");
        //printf("PES stream id: %04X\n", pes->stream_id);
    }
}

void print_pat(TSDemuxContext *ctx, void *data) {
    printf("\n====================\n");
    TSDPATData *pat = (TSDPATData*)data;
    size_t len = pat->length;
    size_t i;
    printf("PAT, Length %d\n", pat->length);

    if(len > 1) {
        printf("number of progs: %d\n", len);
    }
    for(i=0; i<len; ++i) {
        printf("  %d) prog num: 0x%X, pid: 0x%X\n", i, pat->program_number[i], pat->pid[i]);
    }
}

void print_pmt(TSDemuxContext *ctx, void *data) {
    printf("\n====================\n");
    printf("PMT\n");
    TSDPMTData *pmt = (TSDPMTData*)data;
    printf("PCR PID: %04X\n", pmt->pcr_pid);
    printf("program info length: %d\n", pmt->program_info_length);
    printf("descriptors length: %d\n", pmt->descriptors_length);
    size_t i;

    for(i=0;i<pmt->descriptors_length;++i) {
        TSDDescriptor *des = &pmt->descriptors[i];
        printf("  %d) tag: 0x%04X, length: %d\n", i, des->tag, des->length);
        if(des->tag == 0x05) {
            printf("    0x%02x 0x%02x 0x%02x 0x%02x\n", des->data[0], des->data[1], des->data[2], des->data[3]);
        }
    }

    printf("program elements length: %d\n", pmt->program_elements_length);
    for(i=0;i<pmt->program_elements_length; ++i) {
        TSDProgramElement *prog = &pmt->program_elements[i];
        printf("  -----\nProgram #%d\n", i);
        printf("  stream type: %04X\n", prog->stream_type);
        printf("  elementary pid: %04X\n", prog->elementary_pid);
        printf("  es info length: %d\n", prog->es_info_length);
        printf("  descriptors length: %d\n", prog->descriptors_length);
        size_t j;

        for(j=0;j<prog->descriptors_length;++j) {
            TSDDescriptor *des = &prog->descriptors[j];
            printf("    %d) tag: 0x%04X, length: %d\n", j, des->tag, des->length);
        }

        // register all the video PIDs.
        // this will cause the demuxer to call this callback if it finds
        // any PES data associated with this PID.
        if(prog->stream_type == 0x00) {
            tsd_register_pid(ctx, prog->elementary_pid);
        }
    }
}
