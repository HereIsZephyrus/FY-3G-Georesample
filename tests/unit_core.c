#include "test_suites.h"
#include "interface.h"
#include "core.h"

static const char* TEST_INPUT_FILE = "/mnt/repo/hxlc/FY-3G-Georesample/tests/FY3G_PMR--_ORBA_L1_20250519_1924_5000M_V1.HDF";

void test_comprehensive(void){
    HDFDataset dataset;
    TEST_ASSERT_TRUE(ReadHDF5(TEST_INPUT_FILE, &dataset));
    TEST_MESSAGE("Read HDF5 file successfully");
    GeodeticGrid processedGrid;
    unsigned int capacity = dataset.globalAttribute.scanLineCount * SCAN_ANGLE_COUNT * SCAN_HEIGHT_COUNT;
    RStarPointBatch* pointBatch = CreateRStarPointBatch(capacity);
    TEST_ASSERT_TRUE(ProcessDataset(&dataset, &processedGrid, pointBatch));
    TEST_MESSAGE("Process dataset successfully");
    RStarTree tree;
    TEST_ASSERT_TRUE(CreateRStarTree(pointBatch, &tree));
    TEST_MESSAGE("Create RStar tree successfully");
    ClipGridResult finalGrid;
    TEST_ASSERT_TRUE(InterpolateGrid(&dataset, &processedGrid, &tree, &finalGrid));
    TEST_MESSAGE("Interpolate successfully");
    DestroyClipGridResult(&finalGrid);
    DestroyRStarTree(&tree);
    DestroyHDFDataset(&dataset);
    DestroyFinalGrid(&processedGrid);
    DestroyRStarPointBatch(pointBatch);
}