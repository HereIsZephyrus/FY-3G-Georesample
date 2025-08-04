#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "rstartree.h"

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

void DestroyRStarIndex(RStarIndex* index) {
    if (!index) return;
    if (index->spatialIndex && Index_IsValid(index->spatialIndex) != 0){
        Index_Destroy(index->spatialIndex);
        index->spatialIndex = NULL;
    }
    if (index->properties){
        IndexProperty_Destroy(index->properties);
        index->properties = NULL;
    }
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
    double queryPoint[3] = {(double)point->x, (double)point->y, (double)point->z};
    RTError result = Index_InsertData(index->spatialIndex, point->id, queryPoint, queryPoint, 3, (const uint8_t*)NULL, 0);

    return result == RT_None;
}

bool RStarIndex_DeletePoint(RStarIndex* index, const RStarPoint* point) {
    if (!index || !point || !IsRStarIndexValid(index))
        return false;
    double min[3] = {(double)point->x, (double)point->y, (double)point->z};
    double max[3] = {(double)point->x, (double)point->y, (double)point->z};
    RTError result = Index_DeleteData(index->spatialIndex, point->id, min, max, 3);

    return result == RT_None;
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

void RStarIndex_Flush(RStarIndex* index) {
    if (index && index->spatialIndex)
        Index_Flush(index->spatialIndex);
}

void RStarIndex_ClearBuffer(RStarIndex* index) {
    if (index && index->spatialIndex)
        Index_ClearBuffer(index->spatialIndex);
}

RStarPoint* CreateRStarPoint(float x, float y, float z, int64_t id) {
    RStarPoint* point = (RStarPoint*)malloc(sizeof(RStarPoint));
    if (!point) return NULL;

    point->x = x;
    point->y = y;
    point->z = z;
    point->id = id;

    return point;
}

void DestroyRStarPoint(RStarPoint* point) {
    if (point)
        free(point);
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
        if (result->points)
            free(result->points);
        free(result);
    }
}

void FillQueryPointCoordinates(const RStarPoint* points, unsigned int count, SpatialQueryResult* result) {
    if (!result) return;
    result->points = (RStarPoint*)malloc(count * sizeof(RStarPoint));
    for (unsigned int i = 0; i < count; i++){
        result->points[i].x = points[result->ids[i]].x;
        result->points[i].y = points[result->ids[i]].y;
        result->points[i].z = points[result->ids[i]].z;
        result->points[i].h = points[result->ids[i]].h;
        result->points[i].id = result->ids[i];
    }
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
