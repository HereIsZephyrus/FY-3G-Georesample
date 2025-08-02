#ifndef INDEX_H
#define INDEX_H
#define DEFAULT_K_NEIGHBOR 5
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "kdtree.h"
#include "avltree.h"
#include "rstartree.h"

typedef struct {
    RStarPoint* points[2];
    unsigned int capacity;
} RStarPointBatch;

typedef struct {
    KDCalcPoint* points;
    unsigned int count;
    unsigned int capacity;
} PointBatchAtHeight;

RStarPointBatch* CreateRStarPointBatch(unsigned int initialCapacity);
void DestroyRStarPointBatch(RStarPointBatch* batch);
RStarIndex* CreateRStarIndexFromSortedBatch(RStarPointBatch* batch, const unsigned int bandIndex, const BulkLoadConfig* config);
void RStarPointBatch_SortSpatially(RStarPointBatch* batch, const unsigned int bandIndex);

typedef struct {
    RStarIndex** index[2]; // [bandIndex][clipCount]
    KDTree** hindex[2]; // [bandIndex][HEIGHT_COUNT]
    unsigned int forestSize;
    unsigned int *treeSize;
} IndexForest;

RStarIndex* CreateRStarIndexFromBatch(const RStarPointBatch* batch, const unsigned int startIndex, const unsigned int endIndex, const unsigned int bandIndex, const BulkLoadConfig* config);
AVLTree* CreateAVLTreeFromBatch(const RStarPointBatch* pointBatch, const unsigned int startIndex, const unsigned int endIndex, const unsigned int bandIndex);
void DestroyIndexForest(IndexForest* forest);
#endif