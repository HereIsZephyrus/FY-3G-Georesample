#include "core.h"
#include "data.h"
#include "interpolate.h"
#include "geotransfer.h"
#include "index.h"

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
        geodeticGrid->valueArray[bandIndex][index] = sampleGridInfo->measuredArray[heightIndex];
        if (index >= pointBatch->capacity)
            fprintf(stderr, "Point batch capacity is not enough\n");
        else
            pointBatch->points[bandIndex][index] = *CreateRStarPoint(coordinate.l, coordinate.b, coordinate.h, index, NULL, 0);  
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

bool CreateRStarTree(RStarPointBatch* pointBatch, RStarTree* tree){
    bool success = true;
    BulkLoadConfig* config = CreateDefaultBulkLoadConfig(pointBatch->capacity);
    for (unsigned int bandIndex = 0; bandIndex < 2; bandIndex++){
        tree->index[bandIndex] = CreateRStarIndexFromSortedBatch(pointBatch, bandIndex, config);
        if (!tree->index[bandIndex]){
            fprintf(stderr, "Failed to create RStar index for band %d\n", bandIndex);
            success = false;
        }
    }
    DestroyBulkLoadConfig(config);
    return success;
}

bool InterpolateClipGrid(RStarIndex* indexTree, const float* valueArray, ClipGrid* clipGrid){
    for (unsigned int b = 0; b < clipGrid->latitudeCount; b++)
        for (unsigned int l = 0; l < clipGrid->longitudeCount; l++)
            for (unsigned int h = 0; h < clipGrid->heightCount; h++){
                const float latitude = clipGrid->minLatitude + b * clipGrid->latitudeGap;
                const float longitude = clipGrid->minLongitude + l * clipGrid->longitudeGap;
                const float height = clipGrid->minHeight + h * clipGrid->heightGap;
                const double queryPoint[3] = {latitude, longitude, height};
                unsigned int index = b * clipGrid->longitudeCount * clipGrid->heightCount + l * clipGrid->heightCount + h;
                SpatialQueryResult* result = RStarIndex_NearestNeighborQuery(indexTree, queryPoint, 10);
                if (!result){
                    fprintf(stderr, "Failed to query nearest neighbor for clip grid %d, %d, %d\n", b, l, h);
                    return false;
                }
                double sum = 0;
                for (unsigned int i = 0; i < result->count; i++)
                    sum += valueArray[result->ids[i]];
                clipGrid->value[index] = sum / result->count;
            }
    return true;
}

bool InterpolateGrid(const HDFDataset* dataset, const GeodeticGrid* processedGrid, const RStarTree* tree, ClipGridResult* finalGrid){
    if (!InitClipGridArray(dataset, DEFAULT_GRID_SIZE, DEFAULT_MINIMAL_HEIGHT, DEFAULT_HEIGHT_GAP, DEFAULT_HEIGHT_COUNT, finalGrid)){
        fprintf(stderr, "Failed to initialize final grid\n");
        return false;
    }
    unsigned int clipCount = finalGrid->clipCount;
    for (unsigned int bandIndex = 0; bandIndex < 2; bandIndex++)
        for (unsigned int clipIndex = 0; clipIndex < clipCount; clipIndex++){
            if (!InterpolateClipGrid(tree->index[bandIndex], processedGrid->valueArray[bandIndex], &finalGrid->clipGrids[bandIndex][clipIndex])){
                fprintf(stderr, "Failed to interpolate clip grid for band %d, clip %d\n", bandIndex, clipIndex);
                return false;
            }
        }
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