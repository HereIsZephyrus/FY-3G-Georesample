#ifndef INDEX_H
#define INDEX_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <spatialindex/capi/sidx_api.h>

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

typedef struct {
    IndexH spatialIndex;        // libspatialindex的索引句柄
    IndexPropertyH properties;  // 索引属性
    bool isValid;              // 索引是否有效
    unsigned int dimension;    // 维度 (固定为3)
    unsigned int capacity;     // 节点容量
    double fillFactor;         // 填充因子
} RStarIndex;

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
SpatialQueryResult* RStarIndex_ContainmentQuery(RStarIndex* index, const BoundingBox* queryBox);
SpatialQueryResult* RStarIndex_NearestNeighborQuery(RStarIndex* index, const RStarPoint* queryPoint, unsigned int k);
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

#endif