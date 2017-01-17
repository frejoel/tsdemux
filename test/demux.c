#include "test.h"
#include <tsdemux.h>
#include <string.h>

void test_demux(void);

int main(int argc, char **charv)
{
    test_demux();
    return 0;
}

void table_cb(TSDemuxContext *ctx, TableSection *table)
{
    printf("table_cb");
}

void test_demux(void)
{
    test_start("parse_demux");

    TSDemuxContext ctx;
    set_default_context(&ctx);

    FILE *fp = fopen("test/data/02_f.ts", "rb");
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

    test_end();
}
