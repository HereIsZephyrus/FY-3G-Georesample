#ifndef INDEX_H
#define INDEX_H
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "kdtree.h"
#include "avltree.h"
#include "rstartree.h"
#include "data.h"

typedef struct {
    RStarPoint* points;
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
KDCalcPointBatch* ConstructKDCalcPointFromPointBatch(const GeodeticGrid* geodeticGrid);
unsigned int CalcHeightIndex(float height, unsigned int** indices);
void InsertKDCalcPoint(KDCalcPointClip* point, const float latitude, const float longitude, const unsigned int index);

PointBatchAtHeight* CreatePointBatchAtHeight(unsigned int initialCapacity);
void DestroyPointBatchAtHeight(PointBatchAtHeight* batch);

typedef struct {
    RStarIndex** index; // [clipCount]
    KDTree** flatindex; // [hightCount]
    unsigned int RStarForestSize, KDTreeSize;
} IndexForest;

RStarIndex* CreateRStarIndexFromBatch(const PointBatch* batch, const unsigned int startIndex, const unsigned int endIndex, const BulkLoadConfig* config);
AVLTree* CreateAVLTreeFromBatch(const PointBatch* pointBatch, const unsigned int startIndex, const unsigned int endIndex);
KDTree* CreateKDTreeFromBatch(KDCalcPointClip* clip, unsigned int heightIndex);
bool CreateRStarForest(const PointBatch* pointBatch, ClipGridResult* finalGrid, IndexForest* forest);
bool CreateKDTreeForest(const GeodeticGrid* geodeticGrid, IndexForest* forest);
bool CreateIndexForest(const GeodeticGrid* geodeticGrid, const PointBatch* pointBatch, ClipGridResult* finalGrid, IndexForest* forest);
void DestroyIndexForest(IndexForest* forest);
bool ProtentialToInterpolate(double latitude, double longitude, double height, KDTree** flatindexForest);
#endif