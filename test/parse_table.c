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

    //FILE *fp = fopen("test/data/99.ts", "rb");
    FILE *fp = fopen("/media/macgit/random_segments/idx214.ts", "rb");
    test_assert_null(fp, "open 'test/data/99.ts'");
    char buffer[3000000];
    size_t len = fread(buffer, 1, 3000000, fp);

    // stream some TS packets into the parse_table function to see whether it
    // picks up the PAT correctly
    Table pat;
    memset(&pat, 0, sizeof(pat));

    char *ptr = buffer;
    while(ptr < buffer+len) {
        TSCode res;
        res = parse_table(&ctx, &dctx, ptr, TSD_TSPACKET_SIZE * 5, &pat);
        //test_assert_equal(TSD_OK, res, "return TSD_OK when parsing the next set of packets");
        ptr += (TSD_TSPACKET_SIZE * 5);
    }

    test_end();
}
