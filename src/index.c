#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "data.h"
#include "index.h"
#include "interpolate.h"
#include "kdtree.h"

static int compare_points_x(const void* a, const void* b) {
    const RStarPoint* p1 = (const RStarPoint*)a;
    const RStarPoint* p2 = (const RStarPoint*)b;
    
    if (p1->x < p2->x) return -1;
    if (p1->x > p2->x) return 1;
    return 0;
}

static int compare_points_y(const void* a, const void* b) {
    const RStarPoint* p1 = (const RStarPoint*)a;
    const RStarPoint* p2 = (const RStarPoint*)b;
    
    if (p1->y < p2->y) return -1;
    if (p1->y > p2->y) return 1;
    return 0;
}

static unsigned int CalcExactHeightIndex(float height){
    static const float DEFAULT_MAXIMAL_HEIGHT = (DEFAULT_MINIMAL_HEIGHT + DEFAULT_HEIGHT_COUNT * DEFAULT_HEIGHT_GAP);
    if (height < DEFAULT_MINIMAL_HEIGHT)
        return 0;
    if (height > DEFAULT_MAXIMAL_HEIGHT)
        return DEFAULT_HEIGHT_COUNT - 1;
    return (unsigned int)(ceil((height - DEFAULT_MINIMAL_HEIGHT) / DEFAULT_HEIGHT_GAP));
}

unsigned int CalcHeightIndex(float height, unsigned int** indices){
    unsigned int exactIndex = CalcExactHeightIndex(height);
    if (exactIndex >= DEFAULT_HEIGHT_COUNT + 2)
        return 0;
    if (exactIndex < 2){ // 0 to exactIndex + 2
        unsigned int size = exactIndex + 2 + 1;
        *indices = (unsigned int*)malloc(size * sizeof(unsigned int));
        for (unsigned int i = 0; i < size; i++)
            (*indices)[i] = i;
        return size;
    }
    else if (exactIndex > DEFAULT_HEIGHT_COUNT - 2){ // exactIndex - 2 to DEFAULT_HEIGHT_COUNT( total DEFAULT_HEIGHT_COUNT + 1 layers)
        unsigned int size = DEFAULT_HEIGHT_COUNT + 2 - exactIndex + 1;
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

RStarIndex* CreateRStarIndexFromBatch(const PointBatch* batch, const unsigned int startIndex, const unsigned int endIndex, const unsigned int bandIndex, const BulkLoadConfig* config) {
    if (!batch || !config || batch->capacity == 0 || startIndex >= endIndex) {
        fprintf(stderr, "Invalid batch or config for optimized bulk loading\n");
        return NULL;
    }

    unsigned int validPointCount = 0;
    for (unsigned int i = startIndex; i < endIndex; i++)
        if (batch->points[bandIndex][i].h != -1)
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
        RStarPoint* point = &batch->points[bandIndex][i];
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

AVLTree* CreateAVLTreeFromBatch(const PointBatch* pointBatch, const unsigned int startIndex, const unsigned int endIndex, const unsigned int bandIndex){
    if (!pointBatch || pointBatch->capacity == 0 || startIndex >= endIndex)
        return NULL;
    AVLTree* tree = CreateAVLTree();
    if (!tree) return NULL;
    for (unsigned int i = startIndex; i < endIndex; i++){
        RStarPoint* point = &pointBatch->points[bandIndex][i];
        if (point->h == -1) continue;
        InsertAVLTree(tree, point->h, i);
    }
    return tree;
}

void DestroyIndexForest(IndexForest* forest){
    if (!forest) return;
    for (unsigned int bandIndex = 0; bandIndex < 2; bandIndex++){
        for (unsigned int treeIndex = 0; treeIndex < forest->RStarForestSize; treeIndex++)
            if (forest->index[bandIndex][treeIndex])
                DestroyRStarIndex(forest->index[bandIndex][treeIndex]);

        for (unsigned int treeIndex = 0; treeIndex < forest->KDTreeSize; treeIndex++)
            if (forest->flatindex[bandIndex][treeIndex])
                DestroyKDTree(forest->flatindex[bandIndex][treeIndex]);

        free(forest->index[bandIndex]);
        free(forest->flatindex[bandIndex]);
    }
}

PointBatch* CreateRStarPointBatch(unsigned int initialCapacity) {
    PointBatch* batch = (PointBatch*)malloc(sizeof(PointBatch));
    if (!batch) {
        fprintf(stderr, "Failed to allocate memory for PointBatch\n");
        return NULL;
    }
    batch->points[0] = (RStarPoint*)malloc(initialCapacity * sizeof(RStarPoint));
    batch->points[1] = (RStarPoint*)malloc(initialCapacity * sizeof(RStarPoint));
    batch->capacity = initialCapacity;
    if (!batch->points[0] || !batch->points[1]) {
        fprintf(stderr, "Failed to allocate memory for batch points array\n");
        free(batch);
        return NULL;
    }
    return batch;
}

void DestroyRStarPointBatch(PointBatch* batch) {
    if (!batch) return;
    free(batch->points[0]);
    free(batch->points[1]);
    free(batch);
}

void RStarPointBatch_SortSpatially(PointBatch* batch, const unsigned int bandIndex) {
    if (!batch || batch->capacity == 0) return;
    // Sort first by X coordinate (latitude)
    qsort(batch->points[bandIndex], batch->capacity, sizeof(RStarPoint), compare_points_x);
    // Then sort segments by Y coordinate for better spatial locality
    unsigned int segmentSize = (unsigned int)sqrt(batch->capacity);
    if (segmentSize == 0) segmentSize = 1;
    #pragma omp parallel for shared(batch, segmentSize)
    for (unsigned int i = 0; i < batch->capacity; i += segmentSize) {
        unsigned int segmentEnd = (i + segmentSize < batch->capacity) ? i + segmentSize : batch->capacity;
        qsort(&batch->points[bandIndex][i], segmentEnd - i, sizeof(RStarPoint), compare_points_y);
    }
}

RStarIndex* CreateRStarIndexFromSortedBatch(PointBatch* batch, const unsigned int bandIndex, const BulkLoadConfig* config) {
    if (!batch || !config || batch->capacity == 0)
        return NULL;
    RStarPointBatch_SortSpatially(batch, bandIndex);
    return CreateRStarIndexFromBatch(batch, 0, batch->capacity, bandIndex, config);
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
    bool success = true;
    const unsigned int clipCount = finalGrid->clipCount;
    forest->RStarForestSize = clipCount;
    for (unsigned int bandIndex = 0; bandIndex < 2; bandIndex++){
        forest->index[bandIndex] = (RStarIndex**)malloc(clipCount * sizeof(RStarIndex*));
        if (!forest->index[bandIndex]){
            fprintf(stderr, "Failed to allocate memory for RStar index for band %d\n", bandIndex);
            success = false;
        }
        BulkLoadConfig* config = CreateDefaultBulkLoadConfig();
        #pragma omp parallel for shared(pointBatch, finalGrid, bandIndex, clipCount, config) reduction(||:success)
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

bool CreateKDTreeForest(const GeodeticGrid* geodeticGrid, IndexForest* forest){
    forest->KDTreeSize = DEFAULT_HEIGHT_COUNT + 1;
    bool success = true;
    for (unsigned int bandIndex = 0; bandIndex < 2; bandIndex++){
        forest->flatindex[bandIndex] = (KDTree**)malloc(forest->KDTreeSize * sizeof(KDTree*));
        KDCalcPointBatch* points = ConstructKDCalcPointFromPointBatch(geodeticGrid, bandIndex);
        #pragma omp parallel for shared(points, forest, bandIndex) reduction(||:success)
        for (unsigned int heightIndex = 0; heightIndex < forest->KDTreeSize; heightIndex++){
            forest->flatindex[bandIndex][heightIndex] = CreateKDTreeFromBatch(&points->value[heightIndex], heightIndex);
            if (!forest->flatindex[bandIndex][heightIndex]){
                fprintf(stderr, "Failed to create KDTree for band %d, height %d\n", bandIndex, heightIndex);
                success = false;
            }
        }
        DestroyKDCalcPointBatch(points);
    }
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

KDCalcPointBatch* ConstructKDCalcPointFromPointBatch(const GeodeticGrid* geodeticGrid, unsigned int bandIndex){
    KDCalcPointBatch* batch = (KDCalcPointBatch*)malloc(sizeof(KDCalcPointBatch));
    if (!batch) return NULL;
    batch->heightCount = DEFAULT_HEIGHT_COUNT + 1;
    batch->value = (KDCalcPointClip*)malloc(batch->heightCount * sizeof(KDCalcPointClip));
    for (unsigned int h = 0; h < batch->heightCount; h++){        
        batch->value[h].capacity = DEFAULT_KDTREE_CAPACITY;
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
        const float latitude = geodeticGrid->latitudeArray[bandIndex][i];
        const float longitude = geodeticGrid->longitudeArray[bandIndex][i];
        const float height = geodeticGrid->elevationArray[bandIndex][i];
        if (!geodeticGrid->validArray[bandIndex][i]) continue;
        unsigned int *heightIndices = NULL;
        unsigned int size = CalcHeightIndex(height, &heightIndices);
        for (unsigned int j = 0; j < size; j++)
            InsertKDCalcPoint(&batch->value[heightIndices[j]], latitude, longitude, i);
        free(heightIndices);
    }
    return batch;
}

void InsertKDCalcPoint(KDCalcPointClip* clip, const float latitude, const float longitude, const unsigned int index){
    if (clip->count >= clip->capacity){
        clip->capacity *= 2;
        clip->points = (KDCalcPoint*)realloc(clip->points, clip->capacity * sizeof(KDCalcPoint));
    }
    clip->points[clip->count].latitude = latitude;
    clip->points[clip->count].longitude = longitude;
    clip->points[clip->count].id = index;
    clip->points[clip->count].lat_sum = latitude;
    clip->points[clip->count].lon_sum = longitude;
    clip->points[clip->count].lat_square_sum = latitude * latitude;
    clip->points[clip->count].lon_square_sum = longitude * longitude;
    if (clip->count > 0){
        clip->points[clip->count].lat_sum += clip->points[clip->count - 1].lat_sum;
        clip->points[clip->count].lon_sum += clip->points[clip->count - 1].lon_sum;
        clip->points[clip->count].lat_square_sum += clip->points[clip->count - 1].lat_square_sum;
        clip->points[clip->count].lon_square_sum += clip->points[clip->count - 1].lon_square_sum;
    }
    clip->count++;
}

bool ProtentialToInterpolate(double latitude, double longitude, double height, KDTree** flatindexForest){
    static float maxDis = 0.08; // latitude and longitude max distance
    unsigned int *indices = NULL;
    unsigned int size = CalcHeightIndex(height, &indices);
    bool hasProtential = false;
    for (unsigned int i = 0; i < size; i++){
        if (!flatindexForest[indices[i]]){
            fprintf(stderr, "KDTree for height %d is not created\n", indices[i]);
            return false;
        }
        hasProtential |= KDTreeExistWithinDistance(flatindexForest[indices[i]], latitude, longitude, maxDis);
    }
    free(indices);
    return hasProtential;
}