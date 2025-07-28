#include "test_suites.h"
#include "interface.h"
static const char* TEST_FILE = "tests/FY3G_PMR--_ORBA_L1_20250519_1924_5000M_V1.HDF";

void test_readHDF5(void) {
    TEST_ASSERT_EQUAL(true, ReadHDF5(TEST_FILE));
}