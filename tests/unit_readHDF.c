#include "test_suites.h"
#include "interface.h"
#include <string.h>
static const char* TEST_INPUT_FILE = "/mnt/repo/hxlc/FY-3G-Georesample/tests/FY3G_PMR--_ORBA_L1_20250519_1924_5000M_V1.HDF";
static const char* TEST_OUTPUT_FILE = "/mnt/repo/hxlc/FY-3G-Georesample/tests/output.HDF";

void test_readHDF5(void) {
    HDFDataset dataset;
    TEST_ASSERT_EQUAL(true, ReadHDF5(TEST_INPUT_FILE, &dataset));
    FinalGrid finalGrid;
    TEST_ASSERT_EQUAL(true, ConstructFinalGrid(&dataset, &finalGrid));
    DestroyHDFDataset(&dataset);
    TEST_ASSERT_EQUAL(true, WriteHDF5(TEST_OUTPUT_FILE, &finalGrid, &dataset.globalAttribute));
    DestroyFinalGrid(&finalGrid);
}