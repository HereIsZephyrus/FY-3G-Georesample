#include "test_suites.h"
#include <stdbool.h>

static bool test_round;

void setUp(void) {
    srand(time(NULL));
    test_round = false;
}

void tearDown(void) {}

int main(void) {
    UNITY_BEGIN();
    if (test_round){
        RUN_TEST(test_geotransfer);
        RUN_TEST(test_readHDF5);
        RUN_TEST(test_interpolate);
    }
    else {
        RUN_TEST(test_comprehensive);
    }
    return UNITY_END();
}