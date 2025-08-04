#ifndef RSTARTREE_H
#define RSTARTREE_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <spatialindex/capi/sidx_api.h>

typedef struct {
    IndexH spatialIndex;
    IndexPropertyH properties;
    bool isValid;
    unsigned int capacity; // must greater than 32
    unsigned int totalPointCount;
    double fillFactor;
} RStarIndex;

typedef struct {
    float x, y, z, h;
    int64_t id;
} RStarPoint;

typedef struct {
    int64_t* ids;
    RStarPoint* points;
    unsigned int count;
    unsigned int capacity;
} SpatialQueryResult;

typedef struct {
    unsigned int nodeCapacity;
    double fillFactor;
    unsigned int pageSize;
    unsigned int numberOfPages;
    bool enableParallelSort;
} BulkLoadConfig;

BulkLoadConfig* CreateDefaultBulkLoadConfig();
void DestroyBulkLoadConfig(BulkLoadConfig* config);

RStarIndex* CreateRStarIndex(unsigned int capacity, double fillFactor);
void DestroyRStarIndex(RStarIndex* index);
bool IsRStarIndexValid(RStarIndex* index);

bool RStarIndex_InsertPoint(RStarIndex* index, const RStarPoint* point);
bool RStarIndex_DeletePoint(RStarIndex* index, const RStarPoint* point);

SpatialQueryResult* RStarIndex_NearestNeighborQuery(RStarIndex* index, double queryPoint[3], unsigned int k);
void FillQueryPointCoordinates(const RStarPoint* points, unsigned int count, SpatialQueryResult* result);

void RStarIndex_Flush(RStarIndex* index);
void RStarIndex_ClearBuffer(RStarIndex* index);

RStarPoint* CreateRStarPoint(float x, float y, float z, int64_t id);
void DestroyRStarPoint(RStarPoint* point);
SpatialQueryResult* CreateSpatialQueryResult();
void DestroySpatialQueryResult(SpatialQueryResult* result);
#endif // RSTARTREE_H
