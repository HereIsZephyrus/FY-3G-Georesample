#include <string.h>
#include <math.h>
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "index.h"

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

RStarIndex* CreateRStarIndexFromBatch(const RStarPointBatch* batch, const unsigned int startIndex, const unsigned int endIndex, const unsigned int bandIndex, const BulkLoadConfig* config) {
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

AVLTree* CreateAVLTreeFromBatch(const RStarPointBatch* pointBatch, const unsigned int startIndex, const unsigned int endIndex, const unsigned int bandIndex){
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
        for (unsigned int treeIndex = 0; treeIndex < forest->forestSize; treeIndex++){
            if (forest->index[bandIndex][treeIndex]){
                Index_Destroy(forest->index[bandIndex][treeIndex]->spatialIndex);
                IndexProperty_Destroy(forest->index[bandIndex][treeIndex]->properties);
                free(forest->index[bandIndex][treeIndex]);
            }
        }
        free(forest->index[bandIndex]);
    }
    free(forest);
}

RStarPointBatch* CreateRStarPointBatch(unsigned int initialCapacity) {
    RStarPointBatch* batch = (RStarPointBatch*)malloc(sizeof(RStarPointBatch));
    if (!batch) {
        fprintf(stderr, "Failed to allocate memory for RStarPointBatch\n");
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

void DestroyRStarPointBatch(RStarPointBatch* batch) {
    if (!batch) return;
    free(batch->points[0]);
    free(batch->points[1]);
    free(batch);
}

void RStarPointBatch_SortSpatially(RStarPointBatch* batch, const unsigned int bandIndex) {
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

RStarIndex* CreateRStarIndexFromSortedBatch(RStarPointBatch* batch, const unsigned int bandIndex, const BulkLoadConfig* config) {
    if (!batch || !config || batch->capacity == 0)
        return NULL;
    RStarPointBatch_SortSpatially(batch, bandIndex);
    return CreateRStarIndexFromBatch(batch, 0, batch->capacity, bandIndex, config);
}
