#include "test_suites.h"
#include "interface.h"
#include "interpolate.h"

static const char* TEST_INPUT_FILE = "/mnt/repo/hxlc/FY-3G-Georesample/tests/FY3G_PMR--_ORBA_L1_20250519_1924_5000M_V1.HDF";

void test_interpolate(void) {
    HDFDataset dataset;
    TEST_ASSERT_EQUAL(true, ReadHDF5(TEST_INPUT_FILE, &dataset));
    ClipGridResult finalGrid;
    TEST_ASSERT_EQUAL(true, InitClipGridArray(&dataset, 5000 ,100, 200, 60, &finalGrid));
    DestroyHDFDataset(&dataset);
    DestroyClipGridResult(&finalGrid);
}