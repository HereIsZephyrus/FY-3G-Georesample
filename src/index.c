#include <string.h>
#include <math.h>
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "spatialindex/capi/sidx_config.h"
#include "spatialindex/capi/sidx_api.h"
#include "index.h"

#define INITIAL_CAPACITY 4

static int compare_points_x(const void* a, const void* b) {
    const RStarPoint* p1 = (const RStarPoint*)a;
    const RStarPoint* p2 = (const RStarPoint*)b;
    
    if (p1->latitude < p2->latitude) return -1;
    if (p1->latitude > p2->latitude) return 1;
    return 0;
}
static int compare_points_y(const void* a, const void* b) {
    const RStarPoint* p1 = (const RStarPoint*)a;
    const RStarPoint* p2 = (const RStarPoint*)b;
    
    if (p1->longitude < p2->longitude) return -1;
    if (p1->longitude > p2->longitude) return 1;
    return 0;
}

IndexArray* CreateIndexArray() {
    IndexArray *arr = malloc(sizeof(IndexArray));
    if (!arr) return NULL;
    
    arr->indices = malloc(INITIAL_CAPACITY * sizeof(int));
    if (!arr->indices) {
        fprintf(stderr, "Failed to allocate memory for index array\n");
        free(arr);
        return NULL;
    }
    
    arr->size = 0;
    arr->capacity = INITIAL_CAPACITY;
    return arr;
}

void DestroyIndexArray(IndexArray *arr) {
    if (!arr) return;
    if (arr->indices) 
        free(arr->indices);
    free(arr);
}

bool AppendIndex(IndexArray *arr, int index) {
    if (!arr) return false;
    
    if (arr->size >= arr->capacity) { // dynamically increase the capacity
        unsigned int newCapacity = arr->capacity * 2;
        int *newIndices = realloc(arr->indices, newCapacity * sizeof(int));
        if (!newIndices) return false;
        arr->indices = newIndices;
        arr->capacity = newCapacity;
    }
    arr->indices[arr->size++] = index;
    return true;
}

int GetNodeHeight(AVLNode *node) {
    return node ? node->height : 0;
}

int GetNodeBalanceFactor(AVLNode *node) {
    return node ? GetNodeHeight(node->left) - GetNodeHeight(node->right) : 0;
}

void UpdateNodeHeight(AVLNode *node) {
    if (!node) return;
    node->height = 1 + fmax(GetNodeHeight(node->left), GetNodeHeight(node->right));
}

AVLNode* CreateAVLNode(float value, int arrayIndex) {
    AVLNode *node = malloc(sizeof(AVLNode));
    if (!node) return NULL;
    
    node->value = value;
    node->indices = CreateIndexArray();
    if (!node->indices) {
        fprintf(stderr, "Failed to create index array for AVL node\n");
        free(node);
        return NULL;
    }
    
    if (!AppendIndex(node->indices, arrayIndex)) {
        DestroyIndexArray(node->indices);
        fprintf(stderr, "Failed to append index to AVL node\n");
        free(node);
        return NULL;
    }
    
    node->height = 1;
    node->left = NULL;
    node->right = NULL;
    return node;
}

void DestroyAVLNode(AVLNode *node) {
    if (!node) return;
    DestroyIndexArray(node->indices);
    free(node);
}

AVLNode* RotateRight(AVLNode *y) {
    AVLNode *x = y->left;
    AVLNode *T2 = x->right;
    x->right = y;
    y->left = T2;
    UpdateNodeHeight(y);
    UpdateNodeHeight(x);
    return x;
}

AVLNode* RotateLeft(AVLNode *x) {
    AVLNode *y = x->right;
    AVLNode *T2 = y->left;
    y->left = x;
    x->right = T2;
    UpdateNodeHeight(x);
    UpdateNodeHeight(y);
    return y;
}

AVLTree* CreateAVLTree(void) {
    AVLTree *tree = malloc(sizeof(AVLTree));
    if (!tree) return NULL;
    tree->root = NULL;
    tree->nodeCount = 0;
    return tree;
}

void DestroyAVLNodes(AVLNode *node) {
    if (!node) return;
    DestroyAVLNodes(node->left);
    DestroyAVLNodes(node->right);
    DestroyAVLNode(node);
}

void DestroyAVLTree(AVLTree *tree) {
    if (!tree) return;
    DestroyAVLNodes(tree->root);
    free(tree);
}

AVLNode* InsertAVLNode(AVLNode *node, float value, int arrayIndex, bool *success) {
    if (!node) {
        *success = true;
        return CreateAVLNode(value, arrayIndex);
    }
    
    if (fabs(value - node->value) < 1e-9) { // already exists the value
        *success = AppendIndex(node->indices, arrayIndex);
        return node;
    }
    if (value < node->value)
        node->left = InsertAVLNode(node->left, value, arrayIndex, success);
    else
        node->right = InsertAVLNode(node->right, value, arrayIndex, success);
    
    if (!*success){
        fprintf(stderr, "Failed to insert AVL node\n");
        return node;
    }
    
    UpdateNodeHeight(node);
    
    int balance = GetNodeBalanceFactor(node);
    if (balance > 1 && value < node->left->value) // Left Left Case
        return RotateRight(node);
    if (balance < -1 && value > node->right->value) // Right Right Case
        return RotateLeft(node);
    if (balance > 1 && value > node->left->value){ // Left Right Case
        node->left = RotateLeft(node->left);
        return RotateRight(node);
    }
    if (balance < -1 && value < node->right->value){ // Right Left Case
        node->right = RotateRight(node->right);
        return RotateLeft(node);
    }
    return node;
}

bool InsertAVLTree(AVLTree *tree, float value, int arrayIndex) {
    if (!tree) return false;
    
    bool success = false;
    tree->root = InsertAVLNode(tree->root, value, arrayIndex, &success);
    if (success)
        tree->nodeCount++;
    
    return success;
}

AVLNode* SearchAVLNode(AVLNode *node, float value) {
    if (!node || fabs(value - node->value) < 1e-6) 
        return node;
    
    if (value < node->value) 
        return SearchAVLNode(node->left, value);
    else 
        return SearchAVLNode(node->right, value);
}

AVLNode* SearchAVLTree(AVLTree *tree, float value) {
    if (!tree) return NULL;
    return SearchAVLNode(tree->root, value);
}

void AVLNodeRangeQuery(AVLNode *node, float latMin, float latMax, const float *longitudeArray, float lonMin, float lonMax, int **result, unsigned int *count, unsigned int *capacity) {
    if (!node) return;
    if (node->value >= latMin && node->value <= latMax) {
        for (unsigned int i = 0; i < node->indices->size; i++) {
            // check if the longitude value is in the range
            if (longitudeArray[node->indices->indices[i]] < lonMin || longitudeArray[node->indices->indices[i]] > lonMax)
                continue;
            if (*count >= *capacity) {
                *capacity *= 2;
                *result = realloc(*result, *capacity * sizeof(int));
            }
            (*result)[(*count)++] = node->indices->indices[i];
        }
    }
    
    if (node->value > latMin)
        AVLNodeRangeQuery(node->left, latMin, latMax, longitudeArray, lonMin, lonMax, result, count, capacity);
    if (node->value < latMax)
        AVLNodeRangeQuery(node->right, latMin, latMax, longitudeArray, lonMin, lonMax, result, count, capacity);
}

QueryResult* AVLTreeRangeQuery(AVLTree *tree, float latMin, float latMax, const float *longitudeArray, float lonMin, float lonMax){
    if (!tree) return NULL;
    
    QueryResult *result = malloc(sizeof(QueryResult));
    if (!result) return NULL;
    
    unsigned int capacity = INITIAL_CAPACITY;
    result->indices = malloc(capacity * sizeof(int));
    if (!result->indices) {
        free(result);
        return NULL;
    }
    
    result->count = 0;
    AVLNodeRangeQuery(tree->root, latMin, latMax, longitudeArray, lonMin, lonMax, 
                         &result->indices, &result->count, &capacity);
    return result;
}

void DestroyQueryResult(QueryResult *result) {
    if (!result) return;
    free(result->indices);
    free(result);
}

int AVLTreeGetNodeHeight(AVLTree *tree) {
    return tree ? GetNodeHeight(tree->root) : 0;
}

bool AVLTreeIsEmpty(AVLTree *tree) {
    return !tree || !tree->root;
}

AVLTree* CreateAVLTreeFromArray(const float *values, int arraySize) {
    if (!values || arraySize <= 0) return NULL;
    
    AVLTree *tree = CreateAVLTree();
    if (!tree) return NULL;
    
    for (int i = 0; i < arraySize; i++) {
        if (!InsertAVLTree(tree, values[i], i)) {
            DestroyAVLTree(tree);
            return NULL;
        }
    }
    
    return tree;
}

bool AVLNodeValidateBalance(AVLNode *node) {
    if (!node) return true;
    int balance = GetNodeBalanceFactor(node);
    if (abs(balance) > 1) return false;
    return AVLNodeValidateBalance(node->left) && AVLNodeValidateBalance(node->right);
}

bool AVLTreeValidateBalance(AVLTree *tree) {
    if (!tree) return false;
    return AVLNodeValidateBalance(tree->root);
}

int AVLTreeGetHeight(AVLTree *tree) {
    return tree ? GetNodeHeight(tree->root) : 0;
}

RStarIndex* CreateRStarIndex(unsigned int capacity, double fillFactor) {
    RStarIndex* index = (RStarIndex*)malloc(sizeof(RStarIndex));
    if (!index){
        fprintf(stderr, "Failed to allocate memory for RStarIndex\n");
        return NULL;
    }
    index->spatialIndex = NULL;
    index->properties = NULL;
    index->isValid = false;
    index->capacity = capacity;
    index->fillFactor = fillFactor;
    index->properties = IndexProperty_Create();
    if (!index->properties) {
        fprintf(stderr, "Failed to create index properties\n");
        free(index);
        return NULL;
    }

    IndexProperty_SetIndexType(index->properties, RT_RTree);
    IndexProperty_SetIndexVariant(index->properties, RT_Star);
    IndexProperty_SetDimension(index->properties, 3);
    IndexProperty_SetIndexStorage(index->properties, RT_Memory);
    IndexProperty_SetIndexCapacity(index->properties, capacity);
    IndexProperty_SetLeafCapacity(index->properties, capacity);
    IndexProperty_SetFillFactor(index->properties, fillFactor);

    index->spatialIndex = Index_Create(index->properties);
    if (!index->spatialIndex) {
        fprintf(stderr, "Failed to create spatial index\n");
        IndexProperty_Destroy(index->properties);
        free(index);
        return NULL;
    }
    index->isValid = Index_IsValid(index->spatialIndex) != 0;

    return index;
}

// 销毁三维R*树索引
void DestroyRStarIndex(RStarIndex* index) {
    if (!index) return;

    if (index->spatialIndex)
        Index_Destroy(index->spatialIndex);
    if (index->properties)
        IndexProperty_Destroy(index->properties);
    free(index);
}

bool IsRStarIndexValid(RStarIndex* index) {
    if (!index || !index->spatialIndex) {
        fprintf(stderr, "Invalid RStarIndex\n");
        return false;
    }
    return index->isValid && (Index_IsValid(index->spatialIndex) != 0);
}

bool RStarIndex_InsertPoint(RStarIndex* index, const RStarPoint* point) {
    if (!index || !point || !IsRStarIndexValid(index))
        return false;
    double min[3] = {(double)point->latitude, (double)point->longitude, (double)point->height};
    double max[3] = {(double)point->latitude, (double)point->longitude, (double)point->height};
    RTError result = Index_InsertData(index->spatialIndex, point->id, min, max, 3,
            (const uint8_t*)point->userData, point->userDataSize);

    return result == RT_None;
}

bool RStarIndex_InsertBoundingBox(RStarIndex* index, int64_t id, const BoundingBox* bbox, const void* userData, size_t userDataSize) {
    if (!index || !bbox || !IsRStarIndexValid(index))
        return false;
    double min[3] = {(double)bbox->minLatitude, (double)bbox->minLongitude, (double)bbox->minHeight};
    double max[3] = {(double)bbox->maxLatitude, (double)bbox->maxLongitude, (double)bbox->maxHeight};
    RTError result = Index_InsertData(index->spatialIndex, id, min, max, 3,
            (const uint8_t*)userData, userDataSize);

    return result == RT_None;
}

bool RStarIndex_DeletePoint(RStarIndex* index, const RStarPoint* point) {
    if (!index || !point || !IsRStarIndexValid(index))
        return false;
    double min[3] = {(double)point->latitude, (double)point->longitude, (double)point->height};
    double max[3] = {(double)point->latitude, (double)point->longitude, (double)point->height};
    RTError result = Index_DeleteData(index->spatialIndex, point->id, min, max, 3);

    return result == RT_None;
}

bool RStarIndex_DeleteBoundingBox(RStarIndex* index, int64_t id, const BoundingBox* bbox) {
    if (!index || !bbox || !IsRStarIndexValid(index))
        return false;
    double min[3] = {(double)bbox->minLatitude, (double)bbox->minLongitude, (double)bbox->minHeight};
    double max[3] = {(double)bbox->maxLatitude, (double)bbox->maxLongitude, (double)bbox->maxHeight};
    RTError result = Index_DeleteData(index->spatialIndex, id, min, max, 3);

    return result == RT_None;
}

SpatialQueryResult* RStarIndex_IntersectionQuery(RStarIndex* index, const BoundingBox* queryBox) {
    if (!index || !queryBox || !IsRStarIndexValid(index))
        return NULL;

    double min[3] = {(double)queryBox->minLatitude, (double)queryBox->minLongitude, (double)queryBox->minHeight};
    double max[3] = {(double)queryBox->maxLatitude, (double)queryBox->maxLongitude, (double)queryBox->maxHeight};

    int64_t* ids;
    uint64_t nResults;
    RTError result = Index_Intersects_id(index->spatialIndex, min, max, 3, &ids, &nResults);

    if (result != RT_None || nResults == 0)
        return NULL;

    SpatialQueryResult* queryResult = CreateSpatialQueryResult();
    if (!queryResult) {
        Index_Free(ids);
        return NULL;
    }

    queryResult->ids = (int64_t*)malloc(nResults * sizeof(int64_t));
    if (!queryResult->ids) {
        DestroySpatialQueryResult(queryResult);
        Index_Free(ids);
        return NULL;
    }

    memcpy(queryResult->ids, ids, nResults * sizeof(int64_t));
    queryResult->count = (unsigned int)nResults;
    queryResult->capacity = (unsigned int)nResults;
    Index_Free(ids);
    return queryResult;
}

SpatialQueryResult* RStarIndex_NearestNeighborQuery(RStarIndex* index, double queryPoint[3], unsigned int k) {
    if (!index || !queryPoint || !IsRStarIndexValid(index) || k == 0)
        return NULL;

    int64_t* ids;
    uint64_t nResults;
    RTError result = Index_NearestNeighbors_id(index->spatialIndex, queryPoint, queryPoint, 3, &ids, &nResults);

    if (result != RT_None || nResults == 0)
        return NULL;
    if (nResults > k)
        nResults = k;
    SpatialQueryResult* queryResult = CreateSpatialQueryResult();
    if (!queryResult) {
        Index_Free(ids);
        return NULL;
    }

    queryResult->ids = (int64_t*)malloc(nResults * sizeof(int64_t));
    if (!queryResult->ids) {
        DestroySpatialQueryResult(queryResult);
        Index_Free(ids);
        return NULL;
    }

    memcpy(queryResult->ids, ids, nResults * sizeof(int64_t));
    queryResult->count = (unsigned int)nResults;
    queryResult->capacity = (unsigned int)nResults;

    Index_Free(ids);
    return queryResult;
}

unsigned int RStarIndex_IntersectionCount(RStarIndex* index, const BoundingBox* queryBox) {
    if (!index || !queryBox || !IsRStarIndexValid(index))
        return 0;

    double min[3] = {(double)queryBox->minLatitude, (double)queryBox->minLongitude, (double)queryBox->minHeight};
    double max[3] = {(double)queryBox->maxLatitude, (double)queryBox->maxLongitude, (double)queryBox->maxHeight};

    uint64_t nResults;
    RTError result = Index_Intersects_count(index->spatialIndex, min, max, 3, &nResults);

    if (result != RT_None)
        return 0;

    return (unsigned int)nResults;
}

bool RStarIndex_GetBounds(RStarIndex* index, BoundingBox* bounds) {
    if (!index || !bounds || !IsRStarIndexValid(index))
        return false;

    double* pMins, *pMaxs;
    uint32_t nDimension;

    RTError result = Index_GetBounds(index->spatialIndex, &pMins, &pMaxs, &nDimension);

    if (result != RT_None || nDimension != 3)
        return false;

    bounds->minLatitude = pMins[0];
    bounds->minLongitude = pMins[1];
    bounds->minHeight = pMins[2];
    bounds->maxLatitude = pMaxs[0];
    bounds->maxLongitude = pMaxs[1];
    bounds->maxHeight = pMaxs[2];
    Index_Free(pMins);
    Index_Free(pMaxs);

    return true;
}

void RStarIndex_Flush(RStarIndex* index) {
    if (index && index->spatialIndex)
        Index_Flush(index->spatialIndex);
}

void RStarIndex_ClearBuffer(RStarIndex* index) {
    if (index && index->spatialIndex)
        Index_ClearBuffer(index->spatialIndex);
}

RStarPoint* CreateRStarPoint(float latitude, float longitude, float height, int64_t id, const void* userData, size_t userDataSize) {
    RStarPoint* point = (RStarPoint*)malloc(sizeof(RStarPoint));
    if (!point) return NULL;

    point->latitude = latitude;
    point->longitude = longitude;
    point->height = height;
    point->id = id;
    point->userData = NULL;
    point->userDataSize = userDataSize;

    if (userData && userDataSize > 0) {
        point->userData = malloc(userDataSize);
        if (point->userData)
            memcpy(point->userData, userData, userDataSize);
        else
            point->userDataSize = 0;
    }

    return point;
}

void DestroyRStarPoint(RStarPoint* point) {
    if (point) {
        free(point->userData);
        free(point);
    }
}

BoundingBox* CreateBoundingBox(float minLatitude, float minLongitude, float minHeight, 
      float maxLatitude, float maxLongitude, float maxHeight) {
    BoundingBox* bbox = (BoundingBox*)malloc(sizeof(BoundingBox));
    if (!bbox) return NULL;

    bbox->minLatitude = minLatitude;
    bbox->minLongitude = minLongitude;
    bbox->minHeight = minHeight;
    bbox->maxLatitude = maxLatitude;
    bbox->maxLongitude = maxLongitude;
    bbox->maxHeight = maxHeight;

    return bbox;
}

void DestroyBoundingBox(BoundingBox* bbox) {
    free(bbox);
}

SpatialQueryResult* CreateSpatialQueryResult() {
    SpatialQueryResult* result = (SpatialQueryResult*)malloc(sizeof(SpatialQueryResult));
    if (!result) return NULL;

    result->ids = NULL;
    result->points = NULL;
    result->count = 0;
    result->capacity = 0;
    return result;
}

void DestroySpatialQueryResult(SpatialQueryResult* result) {
    if (result) {
        free(result->ids);
        if (result->points) {
            for (unsigned int i = 0; i < result->count; i++)
                free(result->points[i].userData);
            free(result->points);
        }
        free(result);
    }
}

bool BoundingBox_Intersects(const BoundingBox* box1, const BoundingBox* box2) {
    if (!box1 || !box2) return false;
    return !(box1->maxLatitude < box2->minLatitude || box1->minLatitude > box2->maxLatitude ||
            box1->maxLongitude < box2->minLongitude || box1->minLongitude > box2->maxLongitude ||
            box1->maxHeight < box2->minHeight || box1->minHeight > box2->maxHeight);
}

bool BoundingBox_Contains(const BoundingBox* container, const BoundingBox* contained) {
    if (!container || !contained) return false;
    return (container->minLatitude <= contained->minLatitude && container->maxLatitude >= contained->maxLatitude &&
            container->minLongitude <= contained->minLongitude && container->maxLongitude >= contained->maxLongitude &&
            container->minHeight <= contained->minHeight && container->maxHeight >= contained->maxHeight);
}

bool BoundingBox_ContainsPoint(const BoundingBox* box, const RStarPoint* point) {
    if (!box || !point) return false;

    return (point->latitude >= box->minLatitude && point->latitude <= box->maxLatitude &&
            point->longitude >= box->minLongitude && point->longitude <= box->maxLongitude &&
            point->height >= box->minHeight && point->height <= box->maxHeight);
}

void BoundingBox_Expand(BoundingBox* box, const RStarPoint* point) {
    if (!box || !point) return;

    if (point->latitude < box->minLatitude) box->minLatitude = point->latitude;
    if (point->latitude > box->maxLatitude) box->maxLatitude = point->latitude;
    if (point->longitude < box->minLongitude) box->minLongitude = point->longitude;
    if (point->longitude > box->maxLongitude) box->maxLongitude = point->longitude;
    if (point->height < box->minHeight) box->minHeight = point->height;
    if (point->height > box->maxHeight) box->maxHeight = point->height;
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

BulkLoadConfig* CreateDefaultBulkLoadConfig() {
    BulkLoadConfig* config = (BulkLoadConfig*)malloc(sizeof(BulkLoadConfig));
    if (!config) {
        fprintf(stderr, "Failed to allocate memory for BulkLoadConfig\n");
        return NULL;
    }
    config->nodeCapacity = 200;
    config->fillFactor = 0.7;
    config->pageSize = 20000;
    config->numberOfPages = 200;
    config->enableParallelSort = true;
    return config;
}

void DestroyBulkLoadConfig(BulkLoadConfig* config) {
    if (config) {
        free(config);
    }
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

RStarIndex* CreateRStarIndexFromBatch(const RStarPointBatch* batch, const unsigned int startIndex, const unsigned int endIndex, const unsigned int bandIndex, const BulkLoadConfig* config) {
    if (!batch || !config || batch->capacity == 0 || startIndex >= endIndex) {
        fprintf(stderr, "Invalid batch or config for optimized bulk loading\n");
        return NULL;
    }

    unsigned int validPointCount = 0;
    for (unsigned int i = startIndex; i < endIndex; i++)
        if (batch->points[bandIndex][i].height != -1)
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
        if (point->height == -1) continue; // skip invalid point
        ids[validIndex] = point->id;
        mins[validIndex * 3 + 0] = (double)point->latitude;
        mins[validIndex * 3 + 1] = (double)point->longitude;
        mins[validIndex * 3 + 2] = (double)point->height;
        maxs[validIndex * 3 + 0] = (double)point->latitude;
        maxs[validIndex * 3 + 1] = (double)point->longitude;
        maxs[validIndex * 3 + 2] = (double)point->height;
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
    
    // 修正stride参数以匹配实际数据布局
    // 根据libspatialindex源码，访问方式为: mins[i*d_i_stri + j*d_j_stri]
    // 我们的数据布局: mins[validIndex * 3 + coordinate_index]
    // 所以: d_i_stri=3, d_j_stri=1
    IndexH spatialIndex = Index_CreateWithArray(properties, validPointCount, 3, 
                                               1,  // i_stri: ID数组步长
                                               3,  // d_i_stri: 每个点在坐标数组中的步长
                                               1,  // d_j_stri: 坐标维度间的步长
                                               ids, mins, maxs);

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

RStarIndex* CreateRStarIndexFromSortedBatch(RStarPointBatch* batch, const unsigned int bandIndex, const BulkLoadConfig* config) {
    if (!batch || !config || batch->capacity == 0)
        return NULL;
    RStarPointBatch_SortSpatially(batch, bandIndex);
    return CreateRStarIndexFromBatch(batch, 0, batch->capacity, bandIndex, config);
}

void DestroyRStarForest(RStarForest* forest){
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