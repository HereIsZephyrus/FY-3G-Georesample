#include <stdio.h>
#include "interface.h"
#include "core.h"
#include "config.h"

int main(int argc, char *argv[]) {
    /**
     * @brief transfer FY-3G raw HDF5 3D prespirator data to geodetic coordinates
     * @param argv[1]: path to the config file
     * @return 0 if success, -1 if failed
    */
    if (argc != 2){
        printf("Usage: %s <config_file>\n", argv[0]);
        return -1;
    }
    g_config = ReadConfig(argv[1]);
    HDFDataset dataset;
    if (!ReadHDF5(g_config->input_file_name, &dataset)){
        printf("Failed to read HDF5 file\n");
        return -1;
    }
    printf("Read HDF5 file successfully\n");

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
    printf("Process dataset successfully\n");

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
    printf("Init clip result successfully\n");
    DestroyHDFDataset(&dataset);
    DestroyRStarPointBatch(pointBatch);
    
    if (!InterpolateGrid(&processedGrid, &forest, &finalGrid)){
        printf("Failed to interpolate grid\n");
        DestroyIndexForest(&forest);
        DestroyClipGridResult(&finalGrid);
        DestroyGeodeticGrid(&processedGrid);
        return -4;
    }

    printf("Interpolate grid successfully\n");
    if (!WriteClipResult(g_config->clip_output_file_name, &finalGrid)){
        printf("Failed to write clip result\n");
        DestroyIndexForest(&forest);
        DestroyClipGridResult(&finalGrid);
        DestroyGeodeticGrid(&processedGrid);
        return -5;
    }
    printf("Write clip result successfully\n");

    DestroyGeodeticGrid(&processedGrid);
    DestroyIndexForest(&forest);
    DestroyClipGridResult(&finalGrid);
    return 0;
}