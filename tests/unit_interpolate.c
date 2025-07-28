#include "interpolate.h"
#include "test_suites.h"
#include "core.h"
#include "interface.h"

static const char* TEST_INPUT_FILE = "/mnt/repo/hxlc/FY-3G-Georesample/tests/FY3G_PMR--_ORBA_L1_20250519_1924_5000M_V1.HDF";

void test_interpolate(void) {
    HDFDataset dataset;
    TEST_ASSERT_EQUAL(true, ReadHDF5(TEST_INPUT_FILE, &dataset));
    FinalGrid finalGrid;
    TEST_ASSERT_EQUAL(true, ProcessDataset(&dataset, &finalGrid));
    DestroyHDFDataset(&dataset);
}