#include <stdio.h>
#include "interface.h"
#include "core.h"

int main(int argc, char *argv[]) {
    /**
     * @brief transfer FY-3G raw HDF5 3D prespirator data to geodetic coordinates
     * @param argv[1]: path to the raw HDF5 file
     * @param argv[2]: path to the output file
     * @return 0 if success, -1 if failed
    */
    if (argc != 3) {
        printf("Usage: %s <input_file> <output_file>\n", argv[0]);
        return -1;
    }
    const char *input_file = argv[1];
    const char *output_file = argv[2];
    HDFDataset dataset;
    if (!ReadHDF5(input_file, &dataset)){
        printf("Failed to read HDF5 file\n");
        return -1;
    }

    GeodeticGrid processedGrid;
    unsigned int capacity = dataset.globalAttribute.scanLineCount * SCAN_ANGLE_COUNT * SCAN_HEIGHT_COUNT;
    PointBatch* pointBatch = CreateRStarPointBatch(capacity);
    if (!ProcessDataset(&dataset, &processedGrid, pointBatch)){
        printf("Failed to process dataset\n");
        DestroyHDFDataset(&dataset);
        DestroyRStarPointBatch(pointBatch);
        DestroyGeodeticGrid(&processedGrid);
        return -2;
    }
    if (!WriteHDF5(output_file, &processedGrid, &dataset.globalAttribute)){
        printf("Failed to write HDF5 file\n");
    }

    IndexForest forest;
    ClipGridResult finalGrid;
    if (!InitClipResult(&dataset, &processedGrid, pointBatch, &forest, &finalGrid)){
        printf("Failed to init clip result\n");
        DestroyHDFDataset(&dataset);
        DestroyRStarPointBatch(pointBatch);
        DestroyGeodeticGrid(&processedGrid);
        DestroyIndexForest(&forest);
        DestroyClipGridResult(&finalGrid);
        return -3;
    }
    DestroyHDFDataset(&dataset);
    DestroyRStarPointBatch(pointBatch);
    printf("Interpolate grid\n");
    if (!InterpolateGrid(&processedGrid, &forest, &finalGrid)){
        printf("Failed to interpolate grid\n");
        DestroyIndexForest(&forest);
        DestroyClipGridResult(&finalGrid);
        DestroyGeodeticGrid(&processedGrid);
        return -4;
    }
    printf("Interpolate successfully\n");

    DestroyGeodeticGrid(&processedGrid);
    DestroyIndexForest(&forest);
    DestroyClipGridResult(&finalGrid);
    return 0;
}