#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "geotransfer.h"
#include "unity.h"
#define MIN_HEIGHT -50
#define MAX_HEIGHT 200000
#define MIN_LATITUDE -80
#define MAX_LATITUDE 80

static double random_latitude() {
    return (double)rand() / RAND_MAX * (MAX_LATITUDE - MIN_LATITUDE) + MIN_LATITUDE;
}

static double random_height() {
    return (double)rand() / RAND_MAX * (MAX_HEIGHT - MIN_HEIGHT) + MIN_HEIGHT;
}

static double random_longitude() {
    return (double)rand() / RAND_MAX * 360 - 180;
}

void setUp(void) {
    srand(time(NULL));
}

void tearDown(void) {}

void test_geotransfer(void) {
    double latitude, longitude, height;
    for (int i = 0; i < 100; i++) {
        char msg[20];
        sprintf(msg, "Test Suite %d:", i);
        TEST_MESSAGE(msg);
        double in_latitude = random_latitude(), in_longitude = random_longitude(), in_height = random_height();
        TEST_ASSERT_EQUAL(true, IsGeodeticValid(in_latitude, in_longitude, in_height));
        double iter_latitude, iter_longitude, iter_height;
        double lagrange_latitude, lagrange_longitude, lagrange_height;
        double x, y, z;
        TransferGeodeticToCartesian(in_latitude, in_longitude, in_height, &x, &y, &z);
        TransferCartesianToGeodetic(x, y, z, &iter_latitude, &iter_longitude, &iter_height, true);
        TransferCartesianToGeodetic(x, y, z, &lagrange_latitude, &lagrange_longitude, &lagrange_height, false);
        TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(1e-10, in_latitude, iter_latitude, "latitude calculated by iterative method is equal to the input latitude");
        TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(1e-10, in_longitude, iter_longitude, "longitude calculated by iterative method is equal to the input longitude");
        TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(1e-4, in_height, iter_height, "height calculated by iterative method is equal to the input height");
        TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(1e-10, in_latitude, lagrange_latitude, "latitude calculated by Lagrange method is equal to the input latitude");
        TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(1e-10, in_longitude, lagrange_longitude, "longitude calculated by Lagrange method is equal to the input longitude");
        TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(1e-4, in_height, lagrange_height, "height calculated by Lagrange method is equal to the input height");
    }
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_geotransfer);
    return UNITY_END();
}