#ifndef INDEX_H
#define INDEX_H
#define DEFAULT_K_NEIGHBOR 5
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "kdtree.h"
#include "avltree.h"
#include "rstartree.h"
#include "data.h"

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

PointBatchAtHeight* CreatePointBatchAtHeight(unsigned int initialCapacity);
void DestroyPointBatchAtHeight(PointBatchAtHeight* batch);

typedef struct {
    RStarIndex** index[2]; // [bandIndex][clipCount]
    KDTree** flatindex[2]; // [bandIndex][hightCount]
    unsigned int RStarForestSize, KDTreeSize;
} IndexForest;

RStarIndex* CreateRStarIndexFromBatch(const RStarPointBatch* batch, const unsigned int startIndex, const unsigned int endIndex, const unsigned int bandIndex, const BulkLoadConfig* config);
AVLTree* CreateAVLTreeFromBatch(const RStarPointBatch* pointBatch, const unsigned int startIndex, const unsigned int endIndex, const unsigned int bandIndex);
KDTree* CreateKDTreeFromBatch(KDCalcPoint* points, int count, unsigned int heightIndex);
bool CreateRStarForest(const RStarPointBatch* pointBatch, ClipGridResult* finalGrid, IndexForest* forest);
bool CreateKDTreeForest(const RStarPointBatch* pointBatch, ClipGridResult* finalGrid, IndexForest* forest);
bool CreateIndexForest(const RStarPointBatch* pointBatch, ClipGridResult* finalGrid, IndexForest* forest);
void DestroyIndexForest(IndexForest* forest);
#endif