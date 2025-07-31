#include "test_suites.h"
#include "interface.h"
#include "core.h"
#include <string.h>
#include "index.h"

static const char* TEST_INPUT_FILE = "/mnt/repo/hxlc/FY-3G-Georesample/tests/FY3G_PMR--_ORBA_L1_20250519_1924_5000M_V1.HDF";
static const char* TEST_OUTPUT_FILE = "/mnt/repo/hxlc/FY-3G-Georesample/tests/output.HDF";

void test_readHDF5(void) {
    HDFDataset dataset;
    TEST_ASSERT_TRUE(ReadHDF5(TEST_INPUT_FILE, &dataset));
    TEST_MESSAGE("Read HDF5 file successfully");
    GeodeticGrid finalGrid;
    unsigned int initialCapacity = dataset.globalAttribute.scanLineCount * SCAN_ANGLE_COUNT * SCAN_HEIGHT_COUNT;
    RStarPointBatch* pointBatch = CreateRStarPointBatch(initialCapacity);
    TEST_ASSERT_TRUE(ProcessDataset(&dataset, &finalGrid, pointBatch));
    TEST_MESSAGE("Process dataset successfully");
    DestroyHDFDataset(&dataset);
    TEST_ASSERT_TRUE(WriteHDF5(TEST_OUTPUT_FILE, &finalGrid, &dataset.globalAttribute));
    TEST_MESSAGE("Write HDF5 file successfully");
    DestroyFinalGrid(&finalGrid);
    DestroyRStarPointBatch(pointBatch);
}