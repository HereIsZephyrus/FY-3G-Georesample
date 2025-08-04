#include "test_suites.h"
#include "interface.h"
#include "core.h"
#include <string.h>
#include "index.h"

static const char* TEST_INPUT_FILE = "/mnt/repo/hxlc/FY-3G-Georesample/tests/FY3G_PMR--_ORBA_L1_20250519_1924_5000M_V1.HDF";

void test_readHDF5(void) {
    HDFDataset dataset;
    TEST_ASSERT_TRUE(ReadHDF5(0, TEST_INPUT_FILE, &dataset));
    TEST_MESSAGE("Read HDF5 file successfully");
    DestroyHDFDataset(&dataset);
}