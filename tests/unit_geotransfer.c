#include "geotransfer.h"
#include "test_suites.h"
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

void test_geotransfer(void) {
    for (int i = 0; i < 100; i++) {
        char msg[20];
        sprintf(msg, "Test Suite %d:", i);
        TEST_MESSAGE(msg);
        double in_latitude = random_latitude(), in_longitude = random_longitude(), in_height = random_height();
        TEST_ASSERT_TRUE(IsGeodeticValid(in_latitude, in_longitude, in_height));
        double x, y, z;
        TransferGeodeticToCartesian(in_latitude, in_longitude, in_height, &x, &y, &z);
        Coordinate iter_coordinate = TransferCartesianToGeodetic(x, y, z, true);
        Coordinate lagrange_coordinate = TransferCartesianToGeodetic(x, y, z, false);
        TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(1e-10, in_latitude, iter_coordinate.l, "latitude calculated by iterative method is equal to the input latitude");
        TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(1e-10, in_longitude, iter_coordinate.b, "longitude calculated by iterative method is equal to the input longitude");
        TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(1e-4, in_height, iter_coordinate.h, "height calculated by iterative method is equal to the input height");
        TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(1e-10, in_latitude, lagrange_coordinate.l, "latitude calculated by Lagrange method is equal to the input latitude");
        TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(1e-10, in_longitude, lagrange_coordinate.b, "longitude calculated by Lagrange method is equal to the input longitude");
        TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(1e-4, in_height, lagrange_coordinate.h, "height calculated by Lagrange method is equal to the input height");
    }
}