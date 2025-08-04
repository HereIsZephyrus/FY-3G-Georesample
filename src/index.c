#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "data.h"
#include "index.h"
#include "kdtree.h"
#include "config.h"

static unsigned int CalcExactHeightIndex(float height){
    if (height < g_config->minimal_height)
        return 0;
    if (height > g_config->maximal_height)
        return g_config->height_count - 1;
    return (unsigned int)(ceil((height - g_config->minimal_height) / g_config->height_gap));
}

unsigned int CalcHeightIndex(float height, unsigned int** indices){
    /**
    @brief Calculate the height index array at a specific height
    @param height: the height
    @param indices: the indices
    @return the size of the indices
    */
    unsigned int exactIndex = CalcExactHeightIndex(height);
    if (exactIndex >= g_config->height_count + 2)
        return 0;
    if (exactIndex < 2){ // 0 to exactIndex + 2
        unsigned int size = exactIndex + 2 + 1;
        *indices = (unsigned int*)malloc(size * sizeof(unsigned int));
        for (unsigned int i = 0; i < size; i++)
            (*indices)[i] = i;
        return size;
    }
    else if (exactIndex > g_config->height_count - 2){ // exactIndex - 2 to g_config->height_count( total g_config->height_count + 1 layers)
        unsigned int size = g_config->height_count + 2 - exactIndex + 1;
        *indices = (unsigned int*)malloc(size * sizeof(unsigned int));
        for (unsigned int i = 0; i < size; i++)
            (*indices)[i] = exactIndex - 2 + i;
        return size;
    }
    unsigned int size = 5;
    *indices = (unsigned int*)malloc(size * sizeof(unsigned int));
    for (unsigned int i = 0; i < size; i++)
        (*indices)[i] = exactIndex - 2 + i;
    return size;
}

RStarIndex* CreateRStarIndexFromBatch(const PointBatch* batch, const unsigned int startIndex, const unsigned int endIndex, const BulkLoadConfig* config) {
    /**
    @brief Create a RStar index from a batch of points
    @param batch: the batch of points
    @param startIndex: the start index of the batch
    @param endIndex: the end index of the batch
    @param config: the bulk load config
    @return the RStar index
    */
    if (!batch || !config || batch->capacity == 0 || startIndex >= endIndex) {
        fprintf(stderr, "Invalid batch or config for optimized bulk loading\n");
        return NULL;
    }

    unsigned int validPointCount = 0;
    for (unsigned int i = startIndex; i < endIndex; i++)
        if (batch->points[i].h != -1)
            ++validPointCount;
    
    if (validPointCount == 0) {
        fprintf(stderr, "No valid points for bulk loading\n");
        return NULL;
    }

    int64_t* ids = (int64_t*)malloc(validPointCount * sizeof(int64_t));
    double* mins = (double*)malloc(validPointCount * 3 * sizeof(double));
    double* maxs = (double*)malloc(validPointCount * 3 * sizeof(double));
    
    if (!ids || !mins || !maxs) {
        fprintf(stderr, "Failed to allocate arrays for bulk loading\n");
        free(ids);
        free(mins);
        free(maxs);
        return NULL;
    }

    unsigned int validIndex = 0;
    for (unsigned int i = startIndex; i < endIndex; i++) {
        RStarPoint* point = &batch->points[i];
        if (point->h == -1) continue; // skip invalid point
        ids[validIndex] = point->id;
        mins[validIndex * 3 + 0] = (double)point->x;
        mins[validIndex * 3 + 1] = (double)point->y;
        mins[validIndex * 3 + 2] = (double)point->z;
        maxs[validIndex * 3 + 0] = (double)point->x;
        maxs[validIndex * 3 + 1] = (double)point->y;
        maxs[validIndex * 3 + 2] = (double)point->z;
        ++validIndex;
    }

    IndexPropertyH properties = IndexProperty_Create();
    if (!properties) {
        fprintf(stderr, "Failed to create index properties for optimized bulk loading\n");
        free(ids);
        free(mins);
        free(maxs);
        return NULL;
    }

    IndexProperty_SetIndexType(properties, RT_RTree);
    IndexProperty_SetIndexVariant(properties, RT_Star);
    IndexProperty_SetDimension(properties, 3);
    IndexProperty_SetIndexStorage(properties, RT_Memory);
    IndexProperty_SetIndexCapacity(properties, config->nodeCapacity);
    IndexProperty_SetLeafCapacity(properties, config->nodeCapacity);
    IndexProperty_SetFillFactor(properties, config->fillFactor);
    
    IndexH spatialIndex = Index_CreateWithArray(properties, validPointCount, 3, 1, 3,1, ids, mins, maxs);

    free(ids);
    free(mins);
    free(maxs);

    if (!spatialIndex) {
        fprintf(stderr, "Failed to create spatial index using bulk loading\n");
        IndexProperty_Destroy(properties);
        return NULL;
    }

    RStarIndex* index = (RStarIndex*)malloc(sizeof(RStarIndex));
    if (!index) {
        fprintf(stderr, "Failed to allocate memory for RStarIndex wrapper\n");
        Index_Destroy(spatialIndex);
        IndexProperty_Destroy(properties);
        return NULL;
    }

    index->spatialIndex = spatialIndex;
    index->properties = properties;
    index->isValid = Index_IsValid(spatialIndex) != 0;
    index->capacity = config->nodeCapacity;
    index->fillFactor = config->fillFactor;
    index->totalPointCount = validPointCount;
    return index;
}

AVLTree* CreateAVLTreeFromBatch(const PointBatch* pointBatch, const unsigned int startIndex, const unsigned int endIndex){
    if (!pointBatch || pointBatch->capacity == 0 || startIndex >= endIndex)
        return NULL;
    AVLTree* tree = CreateAVLTree();
    if (!tree) return NULL;
    for (unsigned int i = startIndex; i < endIndex; i++){
        RStarPoint* point = &pointBatch->points[i];
        if (point->h == -1) continue;
        InsertAVLTree(tree, point->h, i);
    }
    return tree;
}

void DestroyIndexForest(IndexForest* forest){
    if (!forest) return;
    for (unsigned int treeIndex = 0; treeIndex < forest->RStarForestSize; treeIndex++)
        if (forest->index[treeIndex])
            DestroyRStarIndex(forest->index[treeIndex]);

    for (unsigned int treeIndex = 0; treeIndex < forest->KDTreeSize; treeIndex++)
        if (forest->flatindex[treeIndex])
            DestroyKDTree(forest->flatindex[treeIndex]);

    if (forest->index)
        free(forest->index);
    if (forest->flatindex)
        free(forest->flatindex);
}

PointBatch* CreateRStarPointBatch(unsigned int initialCapacity) {
    /**
    @brief Create an empty point batch
    @param initialCapacity: the initial capacity of the point batch
    @return the point batch
    */
    PointBatch* batch = (PointBatch*)malloc(sizeof(PointBatch));
    if (!batch) {
        fprintf(stderr, "Failed to allocate memory for PointBatch\n");
        return NULL;
    }
    batch->points = (RStarPoint*)malloc(initialCapacity * sizeof(RStarPoint));
    batch->capacity = initialCapacity;
    if (!batch->points) {
        fprintf(stderr, "Failed to allocate memory for batch points array\n");
        free(batch);
        return NULL;
    }
    return batch;
}

void DestroyRStarPointBatch(PointBatch* batch) {
    if (!batch) return;
    free(batch->points);
    free(batch);
}

KDTree* CreateKDTreeFromBatch(KDCalcPointClip* clip, unsigned int heightIndex) {
    KDTree* tree = (KDTree*)malloc(sizeof(KDTree));
    if (!tree) return NULL;

    tree->heightIndex = heightIndex;
    tree->size = clip->count;
    tree->root = BuildKDTree(clip->points, clip->count, 0);
    return tree;
}

bool CreateRStarForest(const PointBatch* pointBatch, ClipGridResult* finalGrid, IndexForest* forest){
    /**
    @brief Create a RStar forest
    @param pointBatch: the point batch
    @param finalGrid: the final grid
    @param forest: the forest
    @return true if the RStar forest is created successfully, false otherwise
    */
    bool success = true;
    const unsigned int clipCount = finalGrid->clipCount;
    forest->RStarForestSize = clipCount;
    BulkLoadConfig* bulkconfig = CreateDefaultBulkLoadConfig();
    forest->index = (RStarIndex**)malloc(clipCount * sizeof(RStarIndex*));
    if (!forest->index){
        fprintf(stderr, "Failed to allocate memory for RStar index\n");
        return false;
    }
    #pragma omp parallel for shared(pointBatch, finalGrid, clipCount, bulkconfig) reduction(||:success) schedule(dynamic)
    for (unsigned int clipIndex = 0; clipIndex < clipCount; clipIndex++){
        const unsigned int startIndex = finalGrid->clipGrids[clipIndex].leftLineIndex * SCAN_ANGLE_COUNT * SCAN_HEIGHT_COUNT;
        const unsigned int endIndex = (finalGrid->clipGrids[clipIndex].rightLineIndex + 1) * SCAN_ANGLE_COUNT * SCAN_HEIGHT_COUNT;
        forest->index[clipIndex] = CreateRStarIndexFromBatch(pointBatch, startIndex, endIndex, bulkconfig);
        if (!forest->index[clipIndex]){
            fprintf(stderr, "Failed to create RStar index for clip %d\n", clipIndex);
            success = false;
        }
    }
    DestroyBulkLoadConfig(bulkconfig);
    return success;
}

bool CreateKDTreeForest(const GeodeticGrid* geodeticGrid, IndexForest* forest){
    /**
    @brief Create a KDTree forest
    @param geodeticGrid: the geodetic grid
    @param forest: the forest
    @return true if the KDTree forest is created successfully, false otherwise
    */
    forest->KDTreeSize = g_config->height_count + 1;
    bool success = true;
    KDCalcPointBatch *points = ConstructKDCalcPointFromPointBatch(geodeticGrid);
    forest->flatindex = (KDTree**)malloc(forest->KDTreeSize * sizeof(KDTree*));
    #pragma omp parallel for shared(points, forest) reduction(||:success) schedule(dynamic)
    for (unsigned int heightIndex = 0; heightIndex < forest->KDTreeSize; heightIndex++){
        forest->flatindex[heightIndex] = CreateKDTreeFromBatch(&points->value[heightIndex], heightIndex);
        if (!forest->flatindex[heightIndex])
            fprintf(stderr, "Failed to create KDTree for height %d\n", heightIndex);
                success = false;
    }
    DestroyKDCalcPointBatch(points);
    return success;
}

bool CreateIndexForest(const GeodeticGrid* geodeticGrid, const PointBatch* pointBatch, ClipGridResult* finalGrid, IndexForest* forest){
    CreateKDTreeForest(geodeticGrid, forest);
    CreateRStarForest(pointBatch, finalGrid, forest);
    return true;
}

void DestroyKDCalcPointBatch(KDCalcPointBatch* batch){
    if (!batch) return;
    for (unsigned int h = 0; h < batch->heightCount; h++){
        if (batch->value[h].points)
            free(batch->value[h].points);
    }
    if (batch->value)
        free(batch->value);
    free(batch);
}

KDCalcPointBatch* ConstructKDCalcPointFromPointBatch(const GeodeticGrid* geodeticGrid){
    KDCalcPointBatch* batch = (KDCalcPointBatch*)malloc(sizeof(KDCalcPointBatch));
    if (!batch) return NULL;
    batch->heightCount = g_config->height_count + 1;
    batch->value = (KDCalcPointClip*)malloc(batch->heightCount * sizeof(KDCalcPointClip));
    for (unsigned int h = 0; h < batch->heightCount; h++){        
        batch->value[h].capacity = g_config->kdtree_capacity;
        batch->value[h].count = 0;
        batch->value[h].points = (KDCalcPoint*)malloc(batch->value[h].capacity * sizeof(KDCalcPoint));
        if (!batch->value[h].points){
            fprintf(stderr, "Failed to allocate memory for KDCalcPoint for height %d\n", h);
            for (unsigned int i = 0; i < h; i++)
                free(batch->value[i].points);
            free(batch->value);
            free(batch);
            return NULL;
        }
    }
    unsigned int capacity = geodeticGrid->lineCount * SCAN_ANGLE_COUNT * geodeticGrid->heightCount;
    for (unsigned int i = 0; i < capacity; i++){
        if (!geodeticGrid->validArray[i]) continue;
        const float latitude = geodeticGrid->latitudeArray[i];
        const float longitude = geodeticGrid->longitudeArray[i];
        const float height = geodeticGrid->elevationArray[i];
        const unsigned int heightIndex = CalcExactHeightIndex(height);
        InsertKDCalcPoint(&batch->value[heightIndex], latitude, longitude, i);
    }
    return batch;
}

void InsertKDCalcPoint(KDCalcPointClip* clip, const float latitude, const float longitude, const unsigned int index){
    /**
    @brief Insert a point into a KDCalcPointClip, which maintain a suffix sum of latitude and longitude to accelerate the calculation of the variance
    @param clip: the KDCalcPointClip
    @param latitude: the latitude of the point
    @param longitude: the longitude of the point
    @param index: the index of the point
    */
    if (clip->count >= clip->capacity){
        clip->capacity *= 2;
        clip->points = (KDCalcPoint*)realloc(clip->points, clip->capacity * sizeof(KDCalcPoint));
    }
    clip->points[clip->count].latitude = latitude;
    clip->points[clip->count].longitude = longitude;
    clip->points[clip->count].id = index;
    clip->count++;
}

bool ProtentialToInterpolate(double latitude, double longitude, double height, KDTree** flatindexForest){
    /**
    @brief Check if the point is potential to be interpolated
    @param latitude: the latitude of the point
    @param longitude: the longitude of the point
    @param height: the height of the point
    @param flatindexForest: the flat index forest
    @return true if the point is potential to be interpolated, false otherwise
    */
    unsigned int *indices = NULL;
    unsigned int size = CalcHeightIndex(height, &indices);
    bool hasProtential = false;
    for (unsigned int i = 0; i < size; i++){
        if (!flatindexForest[indices[i]]){
            fprintf(stderr, "KDTree for height %d is not created\n", indices[i]);
            return false;
        }
        hasProtential |= KDTreeExistWithinDistance(flatindexForest[indices[i]], latitude, longitude, g_config->max_distance_tolerance);
    }
    free(indices);
    return hasProtential;
}