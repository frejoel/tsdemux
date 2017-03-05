#include "test.h"
#include <tsdemux.h>
#include <string.h>

void test_get_version(void);

int main(int argc, char** argv)
{
    test_get_version();
    return 0;
}
void test_get_version(void)
{
    test_start("get_version");

    const char *version = tsd_get_version();
    test_assert_null(version, "version is NULL");
    int len = strlen(version);
    test_assert(len > 0, "version length is 0");
    // the version should be in the format X*.XX*.XX*.
    // look for the two periods.
    int period_count = 0;
    int i=0;
    for(;i<len; ++i) {
        if(version[i] == '.') ++period_count;
    }
    test_assert_equal(period_count, 2, "there should be exaclty 2 periods in the version");

    test_end();
}
