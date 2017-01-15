#include "test.h"
#include <tsdemux.h>
#include <stdio.h>
#include <string.h>

void test_input(void);
void test_pat_parse_stream(void);

int main(int argc, char **argv)
{
    test_input();
    test_pat_parse_stream();
    return 0;
}

void test_input(void)
{

}

void test_pat_parse_stream(void)
{
    test_start("pat_parse stream");

    TSDemuxContext ctx;
    ctx.malloc = malloc;
    ctx.realloc = realloc;
    ctx.calloc = calloc;
    ctx.free = free;

    DataContext dctx;
    memset(&dctx, 0, sizeof(dctx));

    FILE *fp = fopen("test/data/00.ts", "rb");
    test_assert_null(fp, "open 'test/data/00.ts'");
    char buffer[20480]; // 20KB
    size_t len = fread(buffer, 1, 20480, fp);

    // stream some TS packets into the parse_pat function to see whether it
    // picks up the PAT correctly
    PATable pat;
    memset(&pat, 0, sizeof(pat));

    char *ptr = buffer;
    while(ptr < buffer+len) {
        TSCode res;
        res = parse_pat(&ctx, &dctx, ptr, TSD_TSPACKET_SIZE * 5, &pat);
        test_assert_equal(TSD_OK, res, "return TSD_OK when parsing the next set of packets");
        ptr += (TSD_TSPACKET_SIZE * 5);
    }

    test_end();
}
