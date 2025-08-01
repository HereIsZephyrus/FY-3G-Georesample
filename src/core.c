#include "core.h"
#include "data.h"
#include "interpolate.h"
#include "geotransfer.h"
#include "index.h"

bool IsValidHeightData(const float coordinateHeight, const float elevation, const unsigned int heightIndex, const float clutterFreeBottomIndex){
    static const float DEFAULT_MAXIMAL_HEIGHT = (DEFAULT_MINIMAL_HEIGHT + DEFAULT_HEIGHT_COUNT * DEFAULT_HEIGHT_GAP);
    if (heightIndex >= clutterFreeBottomIndex) return false;
    if (coordinateHeight > DEFAULT_MAXIMAL_HEIGHT) return false;
    if (coordinateHeight < elevation) return false;
    return true;
}

void CalculateGridData(const GridInfo* sampleGridInfo, GeodeticGrid* geodeticGrid, RStarPointBatch* pointBatch, unsigned int bandIndex, unsigned int lineIndex, unsigned int angleIndex){
    /**
    @brief Calculate the interpolation
    @param sampleGridInfo: the sample grid info to calculate the interpolation
    @param geodeticGrid: the final grid to store the data
    @param pointBatch: the point batch to store the data
    @param bandIndex: the band index
    @param lineIndex: the line index
    @param angleIndex: the angle index
    */
    double groundX, groundY, groundZ;
    TransferGeodeticToCartesian(sampleGridInfo->groundB, sampleGridInfo->groundL, sampleGridInfo->groundH, &groundX, &groundY, &groundZ);
    CartesianInterpolator interpolator = calcInterParams(groundX, groundY, groundZ, sampleGridInfo->groundH,
                                                        sampleGridInfo->airB, sampleGridInfo->airL, sampleGridInfo->zeta);
    for (unsigned int heightIndex = 0; heightIndex < geodeticGrid->heightCount; heightIndex++){
        Coordinate coordinate = calcCartesian(&interpolator, sampleGridInfo->heightArray[heightIndex]);
        const unsigned int index = lineIndex * SCAN_ANGLE_COUNT * geodeticGrid->heightCount + angleIndex * geodeticGrid->heightCount + heightIndex;
        geodeticGrid->latitudeArray[bandIndex][index] = coordinate.l;
        geodeticGrid->longitudeArray[bandIndex][index] = coordinate.b;
        geodeticGrid->elevationArray[bandIndex][index] = coordinate.h;
        if (sampleGridInfo->measuredArray[heightIndex] > 0 && sampleGridInfo->measuredArray[heightIndex] < 1000)
            geodeticGrid->valueArray[bandIndex][index] = sampleGridInfo->measuredArray[heightIndex];
        else
            geodeticGrid->valueArray[bandIndex][index] = -999; // NaN data
        if (geodeticGrid->valueArray[bandIndex][index] > -999 && IsValidHeightData(coordinate.h, sampleGridInfo->evaluation, heightIndex ,sampleGridInfo->clutterFreeBottomIndex))
            pointBatch->points[bandIndex][index] = *CreateRStarPoint(coordinate.l, coordinate.b, coordinate.h, index, NULL, 0);  
        else
            pointBatch->points[bandIndex][index].height = -1; // to sign the invalid height data
    }
}

bool ProcessDataset(const HDFDataset* dataset, GeodeticGrid* geodeticGrid, RStarPointBatch* pointBatch){
    /**
    @brief Construct final grid
    @param dataset: the dataset to construct the final grid
    @param geodeticGrid: the final grid to store the data
    @param pointBatch: the point batch to store the data
    @return true if successful, false otherwise
    */
    if (!InitGeodeticGrid(geodeticGrid, dataset->globalAttribute.scanLineCount, SCAN_HEIGHT_COUNT)){
        fprintf(stderr, "Failed to initialize final grid\n");
        return false;
    }
    for (int bandIndex = 0; bandIndex < 2; bandIndex++){
        #pragma omp parallel for shared(dataset, geodeticGrid, bandIndex, pointBatch)
        for (unsigned int lineIndex = 0; lineIndex < geodeticGrid->lineCount; lineIndex++)
            for (unsigned int angleIndex = 0; angleIndex < SCAN_ANGLE_COUNT; angleIndex++){
                CalculateGridData( &dataset->infoArray[bandIndex][lineIndex][angleIndex], 
                                        geodeticGrid, pointBatch, bandIndex, lineIndex, angleIndex);
            }
    }
    return true;
}

bool CreateRStarForest(const RStarPointBatch* pointBatch, ClipGridResult* finalGrid, RStarForest* forest){
    bool success = true;
    //const unsigned int clipCount = finalGrid->clipCount;
    const unsigned int clipCount = 1; // for test
    forest->forestSize = clipCount;
    for (unsigned int bandIndex = 0; bandIndex < 2; bandIndex++){
        forest->index[bandIndex] = (RStarIndex**)malloc(clipCount * sizeof(RStarIndex*));
        if (!forest->index[bandIndex]){
            fprintf(stderr, "Failed to allocate memory for RStar index for band %d\n", bandIndex);
            success = false;
        }
        BulkLoadConfig* config = CreateDefaultBulkLoadConfig();
        //#pragma omp parallel for shared(pointBatch, finalGrid, bandIndex, clipCount, config) reduction(||:success)
        for (unsigned int clipIndex = 0; clipIndex < clipCount; clipIndex++){
            const unsigned int startIndex = finalGrid->clipGrids[bandIndex][clipIndex].leftLineIndex * SCAN_ANGLE_COUNT * SCAN_HEIGHT_COUNT;
            const unsigned int endIndex = (finalGrid->clipGrids[bandIndex][clipIndex].rightLineIndex + 1) * SCAN_ANGLE_COUNT * SCAN_HEIGHT_COUNT;
            forest->index[bandIndex][clipIndex] = CreateRStarIndexFromBatch(pointBatch, startIndex, endIndex, bandIndex, config);
            if (!forest->index[bandIndex][clipIndex]){
                fprintf(stderr, "Failed to create RStar index for band %d, clip %d\n", bandIndex, clipIndex);
                success = false;
            }
        }
        DestroyBulkLoadConfig(config);
    }
    return success;
}

bool InterpolateClipGrid(RStarIndex* indexTree, const float* valueArray, ClipGrid* clipGrid){
    if (!indexTree) return false;
    for (unsigned int b = 0; b < clipGrid->latitudeCount; b++)
        for (unsigned int l = 0; l < clipGrid->longitudeCount; l++)
            for (unsigned int h = 0; h < clipGrid->heightCount; h++){
                const float latitude = clipGrid->minLatitude + b * clipGrid->latitudeGap;
                const float longitude = clipGrid->minLongitude + l * clipGrid->longitudeGap;
                const float height = clipGrid->minHeight + h * clipGrid->heightGap;
                double queryPoint[3] = {latitude, longitude, height};
                unsigned int index = b * clipGrid->longitudeCount * clipGrid->heightCount + l * clipGrid->heightCount + h;
                SpatialQueryResult* result = RStarIndex_NearestNeighborQuery(indexTree, queryPoint, 5);
                if (!result){
                    fprintf(stderr, "Failed to query nearest neighbor for clip grid %d, %d, %d\n", b, l, h);
                    return false;
                }
                // Use IDW interpolation with power = 2.0
                clipGrid->value[index] = InterpolateValueIDW(queryPoint, result, valueArray, 2.0f);
                printf("The %u line, %u angle, %u height's value is %f\n", b, l, h, clipGrid->value[index]);
                DestroySpatialQueryResult(result);
            }
    return true;
}

bool InterpolateGrid(const GeodeticGrid* processedGrid, RStarForest* forest, ClipGridResult* finalGrid){
    bool success = true;
    //unsigned int clipCount = finalGrid->clipCount;
    const unsigned int clipCount = 1; // for test
    for (unsigned int bandIndex = 0; bandIndex < 2; bandIndex++)
        //#pragma omp parallel for shared(forest, processedGrid, finalGrid, bandIndex, clipCount) reduction(||:success)
        for (unsigned int clipIndex = 0; clipIndex < clipCount; clipIndex++){
            if (!InterpolateClipGrid(forest->index[bandIndex][clipIndex], processedGrid->valueArray[bandIndex], &finalGrid->clipGrids[bandIndex][clipIndex])){
                fprintf(stderr, "Failed to interpolate clip grid for band %d, clip %d\n", bandIndex, clipIndex);
                success = false;
            }
        }
    return success;
}

bool InitClipResult(const HDFDataset* dataset, const RStarPointBatch* pointBatch, RStarForest* forest, ClipGridResult* finalGrid){
    InitClipGridArray(dataset, DEFAULT_GRID_SIZE, DEFAULT_MINIMAL_HEIGHT, DEFAULT_HEIGHT_GAP, DEFAULT_HEIGHT_COUNT, finalGrid);
    CreateRStarForest(pointBatch, finalGrid, forest);
    return true;
}

/*
bool InterpolateClipGrid(AVLTree* indexTree, const float* longitudeArray, const float* valueArray, ClipGrid* clipGrid){
    for (unsigned int b = 0; b < clipGrid->latitudeCount; b++)
        for (unsigned int l = 0; l < clipGrid->longitudeCount; l++)
            for (unsigned int h = 0; h < clipGrid->heightCount; h++){
                const float latitude = clipGrid->minLatitude + b * clipGrid->latitudeGap;
                const float longitude = clipGrid->minLongitude + l * clipGrid->longitudeGap;
                
            }
    return true;
}

bool Interpolate(const HDFDataset* dataset, const GeodeticGrid* processedGrid, AVLTree* indexTree[2], ClipGridResult* finalGrid){
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
*/