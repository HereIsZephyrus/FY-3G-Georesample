#include "test_suites.h"
#include "interface.h"
#include "interpolate.h"

static const char* TEST_INPUT_FILE = "/mnt/repo/hxlc/FY-3G-Georesample/tests/FY3G_PMR--_ORBA_L1_20250519_1924_5000M_V1.HDF";

void test_interpolate(void) {
    HDFDataset dataset;
    TEST_ASSERT_TRUE(ReadHDF5(TEST_INPUT_FILE, &dataset));
    TEST_MESSAGE("Read HDF5 file successfully");
    ClipGridResult finalGrid;
    TEST_ASSERT_TRUE(InitClipGridArray(&dataset, 5000 ,100, 200, 60, &finalGrid));
    TEST_MESSAGE("Init clip grid array successfully");
    DestroyHDFDataset(&dataset);
    DestroyClipGridResult(&finalGrid);
}