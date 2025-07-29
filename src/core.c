#include "core.h"
#include "data.h"
#include "interpolate.h"
#include "geotransfer.h"

bool InitFinalGrid(const HDFDataset* dataset, GeodeticGrid* finalGrid, const int heightCount){
    /**
    @brief Initialize the final grid
    @param finalGrid: the final grid to initialize
    @return true if successful, false otherwise
    */
    finalGrid->lineCount = dataset->globalAttribute.scanLineCount;
    finalGrid->heightCount = heightCount;
    const int dims[3] = {finalGrid->lineCount, SCAN_ANGLE_COUNT, finalGrid->heightCount};
    const int memSize = dims[0] * dims[1] * dims[2] * sizeof(float);
    for (int bandIndex = 0; bandIndex < 2; bandIndex++){
        finalGrid->latitudeArray[bandIndex] = (float*)malloc(memSize);
        finalGrid->longitudeArray[bandIndex] = (float*)malloc(memSize);
        finalGrid->elevationArray[bandIndex] = (float*)malloc(memSize);
        finalGrid->valueArray[bandIndex] = (float*)malloc(memSize);
        if (!finalGrid->latitudeArray[bandIndex] || !finalGrid->longitudeArray[bandIndex] || !finalGrid->elevationArray[bandIndex] || !finalGrid->valueArray[bandIndex]){
            fprintf(stderr, "Failed to allocate memory for latitudeArray, longitudeArray, elevationArray or valueArray\n");
            return false;
        }
    }
    return true;
}

void CalculateGridData(const GridInfo* sampleGridInfo, GeodeticGrid* finalGrid, unsigned int bandIndex, unsigned int lineIndex, unsigned int angleIndex){
    /**
    @brief Calculate the interpolation
    @param sampleGridInfo: the sample grid info to calculate the interpolation
    @param finalGrid: the final grid to store the data
    @param bandIndex: the band index
    @param lineIndex: the line index
    @param angleIndex: the angle index
    */
    double groundX, groundY, groundZ;
    TransferGeodeticToCartesian(sampleGridInfo->groundB, sampleGridInfo->groundL, sampleGridInfo->groundH, &groundX, &groundY, &groundZ);
    CartesianInterpolator interpolator = calcInterParams(groundX, groundY, groundZ, sampleGridInfo->groundH,
                                                        sampleGridInfo->airB, sampleGridInfo->airL, sampleGridInfo->zeta);
    for (unsigned int heightIndex = 0; heightIndex < finalGrid->heightCount; heightIndex++){
        Coordinate coordinate = calcCartesian(&interpolator, sampleGridInfo->heightArray[heightIndex]);
        const int index = lineIndex * SCAN_ANGLE_COUNT * finalGrid->heightCount + angleIndex * finalGrid->heightCount + heightIndex;
        finalGrid->latitudeArray[bandIndex][index] = coordinate.l;
        finalGrid->longitudeArray[bandIndex][index] = coordinate.b;
        finalGrid->elevationArray[bandIndex][index] = coordinate.h;
        finalGrid->valueArray[bandIndex][index] = sampleGridInfo->measuredArray[heightIndex];
    }
}

bool ProcessDataset(const HDFDataset* dataset, GeodeticGrid* finalGrid){
    /**
    @brief Construct final grid
    @param dataset: the dataset to construct the final grid
    @param finalGrid: the final grid to store the data
    @return true if successful, false otherwise
    */
    if (!InitFinalGrid(dataset, finalGrid, SCAN_HEIGHT_COUNT)){
        fprintf(stderr, "Failed to initialize final grid\n");
        return false;
    }
    for (int bandIndex = 0; bandIndex < 2; bandIndex++){
        #pragma omp parallel for shared(dataset, finalGrid, bandIndex)
        for (unsigned int lineIndex = DEBUG_INDEX; lineIndex < finalGrid->lineCount; lineIndex++)
            for (unsigned int angleIndex = 0; angleIndex < SCAN_ANGLE_COUNT; angleIndex++){
                CalculateGridData( &dataset->infoArray[bandIndex][lineIndex][angleIndex], 
                                        finalGrid, bandIndex, lineIndex, angleIndex);
            }
    }
    return true;
}