#include "test_suites.h"
#include <stdbool.h>

void setUp(void) {
    srand(time(NULL));
}

void tearDown(void) {}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_index);
    RUN_TEST(test_rstar3d);
    RUN_TEST(test_kdtree2d);
    RUN_TEST(test_geotransfer);
    RUN_TEST(test_readHDF5);
    RUN_TEST(test_interpolate);
    return UNITY_END();
}