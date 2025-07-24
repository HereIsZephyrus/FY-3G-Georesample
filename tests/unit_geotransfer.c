#include "geotransfer.h"
#include "unity.h"

void setUp(void) {
    // set up stuff here
}

void tearDown(void) {
    // clean up stuff here
}

void test_geotransfer_iterative(void) {
    double latitude, longitude, height;
    TransferCartesianToGeodetic(0, 0, 0, &latitude, &longitude, &height, false);
    TEST_ASSERT_EQUAL_FLOAT(0, latitude);
    TEST_ASSERT_EQUAL_FLOAT(0, longitude);
    TEST_ASSERT_EQUAL_FLOAT(0, height);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_geotransfer_iterative);
    return UNITY_END();
}