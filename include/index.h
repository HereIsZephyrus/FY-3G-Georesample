#ifndef INDEX_H
#define INDEX_H
#define DEFAULT_K_NEIGHBOR 5
#define DEFAULT_KDTREE_CAPACITY 1000000
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
} PointBatch;

typedef struct{
    KDCalcPoint* points;
    unsigned int count;
    unsigned int capacity;
} KDCalcPointClip;

typedef struct {
    KDCalcPointClip* value;
    unsigned int heightCount;
} KDCalcPointBatch;

typedef struct {
    KDCalcPoint* points;
    unsigned int count;
    unsigned int capacity;
} PointBatchAtHeight;

PointBatch* CreateRStarPointBatch(unsigned int initialCapacity);
void DestroyRStarPointBatch(PointBatch* batch);
void DestroyKDCalcPointBatch(KDCalcPointBatch* batch);
KDCalcPointBatch* ConstructKDCalcPointFromPointBatch(const PointBatch* pointBatch, unsigned int bandIndex);
RStarIndex* CreateRStarIndexFromSortedBatch(PointBatch* batch, const unsigned int bandIndex, const BulkLoadConfig* config);
void RStarPointBatch_SortSpatially(PointBatch* batch, const unsigned int bandIndex);
unsigned int CalcHeightIndex(float height, unsigned int* indices);
void InsertKDCalcPoint(KDCalcPointClip* point, const RStarPoint* rStarPoint);

PointBatchAtHeight* CreatePointBatchAtHeight(unsigned int initialCapacity);
void DestroyPointBatchAtHeight(PointBatchAtHeight* batch);

typedef struct {
    RStarIndex** index[2]; // [bandIndex][clipCount]
    KDTree** flatindex[2]; // [bandIndex][hightCount]
    unsigned int RStarForestSize, KDTreeSize;
} IndexForest;

RStarIndex* CreateRStarIndexFromBatch(const PointBatch* batch, const unsigned int startIndex, const unsigned int endIndex, const unsigned int bandIndex, const BulkLoadConfig* config);
AVLTree* CreateAVLTreeFromBatch(const PointBatch* pointBatch, const unsigned int startIndex, const unsigned int endIndex, const unsigned int bandIndex);
KDTree* CreateKDTreeFromBatch(KDCalcPointClip* clip, unsigned int heightIndex);
bool CreateRStarForest(const PointBatch* pointBatch, ClipGridResult* finalGrid, IndexForest* forest);
bool CreateKDTreeForest(const PointBatch* pointBatch, IndexForest* forest);
bool CreateIndexForest(const PointBatch* pointBatch, ClipGridResult* finalGrid, IndexForest* forest);
void DestroyIndexForest(IndexForest* forest);
#endif