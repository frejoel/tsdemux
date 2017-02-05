#include "test.h"
#include <tsdemux.h>
#include <string.h>

void test_demux(void);

int main(int argc, char **charv)
{
    test_demux();
    return 0;
}

void event_cb(TSDemuxContext *ctx, EventId event_id, void *data)
{
    printf("\n##########\nevent_cb %d\n==========\n", event_id);
    if(event_id == TSD_EVENT_PAT) {
        PATData *pat = (PATData*)data;
        size_t len = pat->length;
        size_t i;
        printf("PAT, Length %d\n", pat->length);

        for(i=0; i<len; ++i) {
            printf("  %d) prog num: 0x%X, pid: 0x%X\n", i, pat->program_number[i], pat->pid[i]);
        }
    }

    if(event_id == TSD_EVENT_PMT) {
        printf("PMT\n");
        PMTData *pmt = (PMTData*)data;
        printf("PCR PID: %04X\n", pmt->pcr_pid);
        printf("program info length: %d\n", pmt->program_info_length);
        printf("descriptors length: %d\n", pmt->descriptors_length);
        size_t i;
        for(i=0;i<pmt->descriptors_length;++i) {
            Descriptor *des = &pmt->descriptors[i];
            printf("  %d) tag: 0x%04X, length: %d\n", i, des->tag, des->length);
            if(des->tag == 0x05) {
                printf("    0x%02x 0x%02x 0x%02x 0x%02x\n", des->data[0], des->data[1], des->data[2], des->data[3]);
            }
        }
        printf("program elements length: %d\n", pmt->program_elements_length);
        for(i=0;i<pmt->program_elements_length; ++i) {
            ProgramElement *prog = &pmt->program_elements[i];
            printf("  -----\nProgram #%d\n", i);
            printf("  stream type: %04X\n", prog->stream_type);
            printf("  elementary pid: %04X\n", prog->elementary_pid);
            printf("  es info length: %d\n", prog->es_info_length);
            printf("  descriptors length: %d\n", prog->descriptors_length);
            size_t j;
            for(j=0;j<prog->descriptors_length;++j) {
                Descriptor *des = &prog->descriptors[j];
                printf("    %d) tag: 0x%04X, length: %d\n", j, des->tag, des->length);
            }
        }
    }
}

void test_demux(void)
{
    test_start("parse_demux");

    TSDemuxContext ctx;
    set_default_context(&ctx);

    set_event_callback(&ctx, event_cb);

    FILE *fp = fopen("test/data/friends.ts", "rb");
    char buffer[1880];

    TSPacket hdr;
    int count = fread(buffer, 1, 1880, fp);
    TSCode res;
    size_t parsed = 0;

    while(count > 0) {
        parsed = demux(&ctx, buffer, count, &res);
        count = fread(buffer, 1, 1880, fp);
    }

    fclose(fp);

    size_t i=0;
    for(i=0; i<ctx.pids_length; ++i) {
        printf("PID %d) 0x%04X\n", i, ctx.pids[i]);
    }

    test_end();
}
