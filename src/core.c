#include "core.h"
#include "data.h"
#include "interpolate.h"
#include "geotransfer.h"

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
    if (!InitGeodeticGrid(finalGrid, dataset->globalAttribute.scanLineCount, SCAN_HEIGHT_COUNT)){
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