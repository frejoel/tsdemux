#include "test.h"

int main()
{
    test_start("test");

    test_assert(1, "something happened to this things");

    test_end();
    return 0;
}
