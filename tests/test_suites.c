#include "test_suites.h"

void setUp(void) {
    srand(time(NULL));
}

void tearDown(void) {}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_geotransfer);
    RUN_TEST(test_readHDF5);
    return UNITY_END();
}