#ifndef INDEX_H
#define INDEX_H
#define DEFAULT_K_NEIGHBOR 5
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <spatialindex/capi/sidx_api.h>

// ================================ AVL Tree (currently aborted) ================================
typedef struct {
    int *indices;
    unsigned int size; // real size
    unsigned int capacity; // capacity
} IndexArray;

typedef struct AVLNode {
    float value;
    IndexArray *indices;
    int height;
    struct AVLNode *left;
    struct AVLNode *right;
} AVLNode;

typedef struct {
    AVLNode *root;
    int nodeCount;
} AVLTree;

typedef struct {
    int *indices;
    unsigned int count;
} QueryResult;

IndexArray* CreateIndexArray();
void DestroyIndexArray(IndexArray *arr);
bool AppendIndex(IndexArray *arr, int index);

int GetNodeHeight(AVLNode *node);
int GetNodeBalanceFactor(AVLNode *node);
void UpdateNodeHeight(AVLNode *node);
AVLNode* CreateAVLNode(float value, int arrayIndex);
void DestroyAVLNode(AVLNode *node);
AVLNode* InsertAVLNode(AVLNode *node, float value, int arrayIndex, bool *success);
AVLNode* RotateRight(AVLNode *y);
AVLNode* RotateLeft(AVLNode *x);

AVLTree* CreateAVLTree(void);
void DestroyAVLTree(AVLTree *tree);
bool InsertAVLTree(AVLTree *tree, float value, int arrayIndex);
AVLNode* SearchAVLTree(AVLTree *tree, float value);
AVLNode* SearchAVLNode(AVLNode *node, float value);

// Range query function
QueryResult* AVLTreeRangeQuery(AVLTree *tree, float latMin, float latMax, const float *longitudeArray, float lonMin, float lonMax);
void AVLNodeRangeQuery(AVLNode *node, float latMin, float latMax, const float *longitudeArray, float lonMin, float lonMax, int **result, unsigned int *count, unsigned int *capacity);
void DestroyQueryResult(QueryResult *result);

int AVLTreeGetHeight(AVLTree *tree);
bool AVLTreeIsEmpty(AVLTree *tree);

AVLTree* CreateAVLTreeFromArray(const float *values, int arraySize);

int AVLNodeGetHeight(AVLNode *node);
int AVLNodeGetBalanceFactor(AVLNode *node);
bool AVLTreeValidateBalance(AVLTree *tree);
bool AVLNodeValidateBalance(AVLNode *node);

// ================================ R* Tree (currently working) ================================
typedef struct {
    IndexH spatialIndex;
    IndexPropertyH properties;
    bool isValid;
    unsigned int capacity; // must greater than 32
    unsigned int totalPointCount;
    double fillFactor;
} RStarIndex;

typedef struct {
    RStarIndex** index[2]; // [bandIndex][clipCount]
    unsigned int forestSize;
    unsigned int *treeSize;
} RStarForest;

typedef struct {
    float x, y, z, h;
    int64_t id;
    void* userData;
    size_t userDataSize;
} RStarPoint;

typedef struct {
    int64_t* ids;
    RStarPoint* points;
    unsigned int count;
    unsigned int capacity;
} SpatialQueryResult;

RStarIndex* CreateRStarIndex(unsigned int capacity, double fillFactor);
void DestroyRStarIndex(RStarIndex* index);
bool IsRStarIndexValid(RStarIndex* index);

bool RStarIndex_InsertPoint(RStarIndex* index, const RStarPoint* point);
bool RStarIndex_DeletePoint(RStarIndex* index, const RStarPoint* point);

SpatialQueryResult* RStarIndex_NearestNeighborQuery(RStarIndex* index, double queryPoint[3], unsigned int k);
void FillQueryPointCoordinates(const RStarPoint* points, unsigned int count, SpatialQueryResult* result);

void RStarIndex_Flush(RStarIndex* index);
void RStarIndex_ClearBuffer(RStarIndex* index);

RStarPoint* CreateRStarPoint(float x, float y, float z, int64_t id, const void* userData, size_t userDataSize);
void DestroyRStarPoint(RStarPoint* point);
SpatialQueryResult* CreateSpatialQueryResult();
void DestroySpatialQueryResult(SpatialQueryResult* result);

typedef struct {
    RStarPoint* points[2];
    unsigned int capacity;
} RStarPointBatch;

typedef struct {
    unsigned int nodeCapacity;
    double fillFactor;
    unsigned int pageSize;
    unsigned int numberOfPages;
    bool enableParallelSort;
} BulkLoadConfig;

RStarPointBatch* CreateRStarPointBatch(unsigned int initialCapacity);
void DestroyRStarPointBatch(RStarPointBatch* batch);

RStarIndex* CreateRStarIndexFromBatch(const RStarPointBatch* batch, const unsigned int startIndex, const unsigned int endIndex, const unsigned int bandIndex, const BulkLoadConfig* config);
BulkLoadConfig* CreateDefaultBulkLoadConfig();
void DestroyBulkLoadConfig(BulkLoadConfig* config);

RStarIndex* CreateRStarIndexFromSortedBatch(RStarPointBatch* batch, const unsigned int bandIndex, const BulkLoadConfig* config);
void RStarPointBatch_SortSpatially(RStarPointBatch* batch, const unsigned int bandIndex);
void DestroyRStarForest(RStarForest* forest);
#endif