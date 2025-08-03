#include <omp.h>
#include "core.h"
#include "data.h"
#include "interpolate.h"
#include "geotransfer.h"
#include "index.h"

static bool IsValidHeightData(const float coordinateHeight, const float elevation, const unsigned int heightIndex, const float clutterFreeBottomIndex){
    static const float DEFAULT_MAXIMAL_HEIGHT = (DEFAULT_MINIMAL_HEIGHT + DEFAULT_HEIGHT_COUNT * DEFAULT_HEIGHT_GAP);
    if (heightIndex >= clutterFreeBottomIndex) return false;
    if (coordinateHeight > DEFAULT_MAXIMAL_HEIGHT) return false;
    if (coordinateHeight < elevation) return false;
    return true;
}

void CalculateGridData(const GridInfo* sampleGridInfo, GeodeticGrid* geodeticGrid, PointBatch* pointBatch, unsigned int bandIndex, unsigned int lineIndex, unsigned int angleIndex){
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
        Coordinate coordinate = CalcCartesian(&interpolator, sampleGridInfo->heightArray[heightIndex]);
        const unsigned int index = lineIndex * SCAN_ANGLE_COUNT * geodeticGrid->heightCount + angleIndex * geodeticGrid->heightCount + heightIndex;
        geodeticGrid->latitudeArray[bandIndex][index] = coordinate.l;
        geodeticGrid->longitudeArray[bandIndex][index] = coordinate.b;
        geodeticGrid->elevationArray[bandIndex][index] = coordinate.h;
        if (sampleGridInfo->measuredArray[heightIndex] > 0 && sampleGridInfo->measuredArray[heightIndex] < 1000)
            geodeticGrid->valueArray[bandIndex][index] = sampleGridInfo->measuredArray[heightIndex];
        else{
            geodeticGrid->valueArray[bandIndex][index] = -999; // NaN data
            geodeticGrid->validArray[bandIndex][index] = false;
        }
        if (geodeticGrid->valueArray[bandIndex][index] > -999 && IsValidHeightData(coordinate.h, sampleGridInfo->evaluation, heightIndex ,sampleGridInfo->clutterFreeBottomIndex)){
            pointBatch->points[bandIndex][index] = *CreateRStarPoint(coordinate.x, coordinate.y, coordinate.z, index, NULL, 0);  
            pointBatch->points[bandIndex][index].h = coordinate.h;
        }
        else{
            pointBatch->points[bandIndex][index].h = -1; // to sign the invalid height data
            geodeticGrid->validArray[bandIndex][index] = false;
        }
    }
}

bool ProcessDataset(const HDFDataset* dataset, GeodeticGrid* geodeticGrid, PointBatch* pointBatch){
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

bool InterpolateClipGrid(const RStarPoint* points, KDTree** flatindexForest, RStarIndex* indexTree, const float* valueArray, ClipGrid* clipGrid){
    /**
    @brief Interpolate the clip grid
    @param points: the points to interpolate
    @param flatindexForest: the KD tree slide by height index
    @param indexTree: the R* tree index
    @param valueArray: the value array
    @param clipGrid: the clip grid
    */
    if (!indexTree || !flatindexForest){
        fprintf(stderr, "Failed to interpolate clip grid, index tree or flatindex tree is NULL\n");
        return false;
    }
    for (unsigned int b = 0; b < clipGrid->latitudeCount; b++)
        for (unsigned int l = 0; l < clipGrid->longitudeCount; l++)
            for (unsigned int h = 0; h < clipGrid->heightCount; h++){
                const float latitude = clipGrid->minLatitude + b * clipGrid->latitudeGap;
                const float longitude = clipGrid->minLongitude + l * clipGrid->longitudeGap;
                const float height = clipGrid->minHeight + h * clipGrid->heightGap;
                unsigned int index = b * clipGrid->longitudeCount * clipGrid->heightCount + l * clipGrid->heightCount + h;
                if (ProtentialToInterpolate(latitude, longitude, height, flatindexForest)){
                    double queryPoint[3];
                    TransferGeodeticToCartesian(latitude, longitude, height, &queryPoint[0], &queryPoint[1], &queryPoint[2]);
                    SpatialQueryResult* result = RStarIndex_NearestNeighborQuery(indexTree, queryPoint, DEFAULT_K_NEIGHBOR);
                    FillQueryPointCoordinates(points, result->count, result);
                    if (!result){
                        fprintf(stderr, "Failed to query nearest neighbor for clip grid %d, %d, %d\n", b, l, h);
                        return false;
                    }
                    clipGrid->value[index] = (float)InterpolateValueIDW(queryPoint, height, result, valueArray, 2.0f);
                    //printf("The %u line, %u angle, %u height's value is %f\n", b, l, h, clipGrid->value[index]);
                    DestroySpatialQueryResult(result);
                }
                else
                    clipGrid->value[index] = -999;
            }
    return true;
}

bool InterpolateClipGridBatch(RStarIndex* indexTree, KDTree** flatindexForest, const float* valueArray, ClipGrid* clipGrid){
    /**
     * @brief Batch version of InterpolateClipGrid using bulk query APIs for improved efficiency
     * @param indexTree: the R* tree index
     * @param flatindexForest: the KD tree slide by height index
     * @param valueArray: array of values to interpolate
     * @param clipGrid: the clip grid to interpolate
     * @return true if successful, false otherwise
     */
    if (!indexTree || !clipGrid || !valueArray || !flatindexForest) return false;
    
    unsigned int capacity = clipGrid->latitudeCount * clipGrid->longitudeCount * clipGrid->heightCount / 4;
    unsigned int totalPoints = 0;
    if (capacity == 0) return false;
    
    double* queryPoints = (double*)malloc(capacity * 3 * sizeof(double));
    double* queryHeights = (double*)malloc(capacity * sizeof(double));
    unsigned int* queryIDs = (unsigned int*)malloc(capacity * sizeof(unsigned int));
    if (!queryPoints || !queryHeights || !queryIDs) {
        fprintf(stderr, "Failed to allocate memory for batch query points\n");
        return false;
    }
    
    for (unsigned int b = 0; b < clipGrid->latitudeCount; b++) {
        for (unsigned int l = 0; l < clipGrid->longitudeCount; l++) {
            for (unsigned int h = 0; h < clipGrid->heightCount; h++) {
                const float latitude = clipGrid->minLatitude + b * clipGrid->latitudeGap;
                const float longitude = clipGrid->minLongitude + l * clipGrid->longitudeGap;
                const float height = clipGrid->minHeight + h * clipGrid->heightGap;
                const unsigned int index = b * clipGrid->longitudeCount * clipGrid->heightCount + l * clipGrid->heightCount + h;
                if (ProtentialToInterpolate(latitude, longitude, height, flatindexForest)){
                    if (totalPoints >= capacity){
                        capacity *= 2;
                        queryPoints = (double*)realloc(queryPoints, capacity * 3 * sizeof(double));
                        queryHeights = (double*)realloc(queryHeights, capacity * sizeof(double));
                        queryIDs = (unsigned int*)realloc(queryIDs, capacity * sizeof(unsigned int));
                    }
                    const unsigned int queryIndex = totalPoints++;
                    TransferGeodeticToCartesian(latitude, longitude, height, &queryPoints[queryIndex * 3 + 0], &queryPoints[queryIndex * 3 + 1], &queryPoints[queryIndex * 3 + 2]);
                    queryHeights[queryIndex] = height;
                    queryIDs[queryIndex] = index;
                }else
                    clipGrid->value[index] = -999;
            }
        }
    }
    
    int64_t* resultIds = (int64_t*)malloc(totalPoints * DEFAULT_K_NEIGHBOR * sizeof(int64_t));
    double* resultDistances = (double*)malloc(totalPoints * DEFAULT_K_NEIGHBOR * sizeof(double));
    uint64_t* resultCounts = (uint64_t*)malloc(totalPoints * sizeof(uint64_t));
    
    if (!resultIds || !resultDistances || !resultCounts) {
        fprintf(stderr, "Failed to allocate memory for batch query results\n");
        if (queryPoints) free(queryPoints);
        if (queryHeights) free(queryHeights);
        if (resultIds) free(resultIds);
        if (resultDistances) free(resultDistances);
        if (resultCounts) free(resultCounts);
        return false;
    }
    
    // Perform batch nearest neighbor query using the new bulk API from PR #268
    int64_t actualProcessed = 0;
    RTError result = Index_NearestNeighbors_id_v(indexTree->spatialIndex,
                                                 DEFAULT_K_NEIGHBOR,    // knn: number of nearest neighbors to find
                                                 totalPoints,           // n: number of query points
                                                 3,                     // d: dimension (latitude, longitude, height)
                                                 totalPoints * DEFAULT_K_NEIGHBOR, // idsz: total size of ids array
                                                 3,                     // d_i_stri: stride between query points
                                                 1,                     // d_j_stri: stride between dimensions
                                                 queryPoints,           // mins: query point coordinates
                                                 queryPoints,           // maxs: same as mins for point queries
                                                 resultIds,             // ids: output array for result ids
                                                 resultCounts,          // cnts: count of results per query point
                                                 resultDistances,       // dists: distances (optional)
                                                 &actualProcessed);     // nr: actual number of processed queries
    
    if (result != RT_None) {
        fprintf(stderr, "Batch nearest neighbor query failed\n");
        free(queryPoints);
        free(queryHeights);
        free(resultIds);
        free(resultDistances);
        free(resultCounts);
        return false;
    }
    
    if (actualProcessed != totalPoints)
        fprintf(stderr, "Warning: Only %ld out of %u queries were processed in batch\n", actualProcessed, totalPoints);
        
    for (unsigned int i = 0; i < totalPoints; i++){
        const unsigned int count = resultCounts[i];
        const unsigned int index = queryIDs[i];
        clipGrid->value[index] = (float)InterpolateValueIDW_v(count, resultDistances, resultIds, valueArray, 2.0f);
    }
    
    if (queryPoints) free(queryPoints);
    if (queryHeights) free(queryHeights);
    if (queryIDs) free(queryIDs);
    if (resultIds) free(resultIds);
    if (resultDistances) free(resultDistances);
    if (resultCounts) free(resultCounts);
    
    return true;
}

bool InterpolateGrid(const GeodeticGrid* processedGrid, IndexForest* forest, ClipGridResult* finalGrid){
    bool success = true;
    unsigned int clipCount = finalGrid->clipCount;
    for (unsigned int bandIndex = 0; bandIndex < 2; bandIndex++)
        #pragma omp parallel for shared(forest, processedGrid, finalGrid, bandIndex, clipCount) reduction(||:success)
        for (unsigned int clipIndex = 0; clipIndex < clipCount; clipIndex++){
            if (!InterpolateClipGridBatch(forest->index[bandIndex][clipIndex], forest->flatindex[bandIndex], processedGrid->valueArray[bandIndex], &finalGrid->clipGrids[bandIndex][clipIndex])){
                fprintf(stderr, "Failed to interpolate clip grid for band %d, clip %d\n", bandIndex, clipIndex);
                success = false;
            }
        }
    return success;
}

bool InitClipResult(const HDFDataset* dataset, const GeodeticGrid* geodeticGrid, const PointBatch* pointBatch, IndexForest* forest, ClipGridResult* finalGrid){
    InitClipGridArray(dataset, DEFAULT_GRID_SIZE, DEFAULT_MINIMAL_HEIGHT, DEFAULT_HEIGHT_GAP, DEFAULT_HEIGHT_COUNT, finalGrid);
    CreateIndexForest(geodeticGrid, pointBatch, finalGrid, forest);
    return true;
}