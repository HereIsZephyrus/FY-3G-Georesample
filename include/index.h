#ifndef INDEX_H
#define INDEX_H

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
    double fillFactor;
} RStarIndex;

typedef struct {
    RStarIndex* index[2];
} RStarTree;

typedef struct {
    float latitude, longitude, height;
    int64_t id;
    void* userData;
    size_t userDataSize;
} RStarPoint;

typedef struct {
    float minLatitude, minLongitude, minHeight;
    float maxLatitude, maxLongitude, maxHeight;
} BoundingBox;

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
bool RStarIndex_InsertBoundingBox(RStarIndex* index, int64_t id, const BoundingBox* bbox, 
                                   const void* userData, size_t userDataSize);
bool RStarIndex_DeletePoint(RStarIndex* index, const RStarPoint* point);
bool RStarIndex_DeleteBoundingBox(RStarIndex* index, int64_t id, const BoundingBox* bbox);

SpatialQueryResult* RStarIndex_IntersectionQuery(RStarIndex* index, const BoundingBox* queryBox);
SpatialQueryResult* RStarIndex_NearestNeighborQuery(RStarIndex* index, const double queryPoint[3], unsigned int k);
unsigned int RStarIndex_IntersectionCount(RStarIndex* index, const BoundingBox* queryBox);

bool RStarIndex_GetBounds(RStarIndex* index, BoundingBox* bounds);
void RStarIndex_Flush(RStarIndex* index);
void RStarIndex_ClearBuffer(RStarIndex* index);

RStarPoint* CreateRStarPoint(float latitude, float longitude, float height, int64_t id, const void* userData, size_t userDataSize);
void DestroyRStarPoint(RStarPoint* point);
BoundingBox* CreateBoundingBox(float minLatitude, float minLongitude, float minHeight, 
                                   float maxLatitude, float maxLongitude, float maxHeight);
void DestroyBoundingBox(BoundingBox* bbox);
SpatialQueryResult* CreateSpatialQueryResult();
void DestroySpatialQueryResult(SpatialQueryResult* result);

bool BoundingBox_Intersects(const BoundingBox* box1, const BoundingBox* box2);
bool BoundingBox_Contains(const BoundingBox* container, const BoundingBox* contained);
bool BoundingBox_ContainsPoint(const BoundingBox* box, const RStarPoint* point);
float RStarPoint_Distance(const RStarPoint* p1, const RStarPoint* p2);
void BoundingBox_Expand(BoundingBox* box, const RStarPoint* point);

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

RStarIndex* CreateRStarIndexFromBatch(const RStarPointBatch* batch, const unsigned int bandIndex, const BulkLoadConfig* config);
BulkLoadConfig* CreateDefaultBulkLoadConfig(unsigned int expectedDataSize);
void DestroyBulkLoadConfig(BulkLoadConfig* config);

RStarIndex* CreateRStarIndexFromSortedBatch(RStarPointBatch* batch, const unsigned int bandIndex, const BulkLoadConfig* config);
void RStarPointBatch_SortSpatially(RStarPointBatch* batch, const unsigned int bandIndex);
void DestroyRStarTree(RStarTree* tree);
#endif