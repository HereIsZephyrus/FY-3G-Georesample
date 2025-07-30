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
        for (unsigned int lineIndex = 0; lineIndex < finalGrid->lineCount; lineIndex++)
            for (unsigned int angleIndex = 0; angleIndex < SCAN_ANGLE_COUNT; angleIndex++){
                CalculateGridData( &dataset->infoArray[bandIndex][lineIndex][angleIndex], 
                                        finalGrid, bandIndex, lineIndex, angleIndex);
            }
    }
    return true;
}

bool InterpolateClipGrid(AVLTree* indexTree, const float* longitudeArray, const float* valueArray, ClipGrid* clipGrid){
    /**
    @brief Interpolate the clip grid
    @param indexTree: the index tree to store the data
    @param longitudeArray: the longitude array to store the data
    @param valueArray: the value array to store the data
    @param clipGrid: the clip grid to store the data
    @return true if successful, false otherwise
    */
    for (unsigned int b = 0; b < clipGrid->latitudeCount; b++)
        for (unsigned int l = 0; l < clipGrid->longitudeCount; l++)
            for (unsigned int h = 0; h < clipGrid->heightCount; h++){
                const float latitude = clipGrid->minLatitude + b * clipGrid->latitudeGap;
                const float longitude = clipGrid->minLongitude + l * clipGrid->longitudeGap;
                
            }
    return true;
}

bool Interpolate(const HDFDataset* dataset, const GeodeticGrid* processedGrid, AVLTree* indexTree[2], ClipGridResult* finalGrid){
    /**
    @brief Interpolate the data
    @param dataset: the dataset to interpolate the data
    @param processedGrid: the processed grid to interpolate the data
    @param indexTree: the index tree to store the data
    @param finalGrid: the final grid to store the data
    @return true if successful, false otherwise
    */
    if (!InitClipGridArray(dataset, DEFAULT_GRID_SIZE, DEFAULT_MINIMAL_HEIGHT, DEFAULT_HEIGHT_GAP, DEFAULT_HEIGHT_COUNT, finalGrid)){
        fprintf(stderr, "Failed to initialize final grid\n");
        return false;
    }
    const unsigned int clipCount = finalGrid->clipCount;
    for (int bandIndex = 0; bandIndex < 2; bandIndex++){
        if (!indexTree[bandIndex]){
            indexTree[bandIndex] = CreateAVLTreeFromArray(processedGrid->latitudeArray[bandIndex], processedGrid->lineCount * SCAN_ANGLE_COUNT * processedGrid->heightCount);
            if (!indexTree[bandIndex]){
                fprintf(stderr, "Failed to create AVL tree\n");
                return false;
            }
        }
        for (unsigned int clipIndex = 0; clipIndex < clipCount; clipIndex++){
            if (!InterpolateClipGrid(indexTree[bandIndex], processedGrid->longitudeArray[bandIndex], processedGrid->valueArray[bandIndex], &finalGrid->clipGrids[bandIndex][clipIndex])){
                fprintf(stderr, "Failed to interpolate clip grid\n");
                return false;
            }
        }
    }
    return true;
}