#include "munit/munit.h"
#include "../src/cpu.h"

int main(void)
{
    CPU cpu;
    (void)cpu;
    munit_assert_not_null(NULL);
    return 0;
}
