#include "../include/index.h"
#include <string.h>
#include <math.h>

// 创建三维R*树索引
RStar3DIndex* CreateRStar3DIndex(RTStorageType storageType, const char* fileName, 
                                 unsigned int capacity, double fillFactor) {
    RStar3DIndex* index = (RStar3DIndex*)malloc(sizeof(RStar3DIndex));
    if (!index) {
        return NULL;
    }
    
    // 初始化结构体
    index->spatialIndex = NULL;
    index->properties = NULL;
    index->isValid = false;
    index->dimension = 3;  // 固定为3维
    index->capacity = capacity;
    index->fillFactor = fillFactor;
    index->storageType = storageType;
    index->fileName = NULL;
    
    // 复制文件名
    if (fileName && storageType == RT_Disk) {
        index->fileName = (char*)malloc(strlen(fileName) + 1);
        if (!index->fileName) {
            free(index);
            return NULL;
        }
        strcpy(index->fileName, fileName);
    }
    
    // 创建索引属性
    index->properties = IndexProperty_Create();
    if (!index->properties) {
        free(index->fileName);
        free(index);
        return NULL;
    }
    
    // 设置索引属性
    IndexProperty_SetIndexType(index->properties, RT_RTree);
    IndexProperty_SetIndexVariant(index->properties, RT_Star);  // R*树变体
    IndexProperty_SetDimension(index->properties, 3);
    IndexProperty_SetIndexStorage(index->properties, storageType);
    IndexProperty_SetIndexCapacity(index->properties, capacity);
    IndexProperty_SetLeafCapacity(index->properties, capacity);
    IndexProperty_SetFillFactor(index->properties, fillFactor);
    
    if (fileName && storageType == RT_Disk) {
        IndexProperty_SetFileName(index->properties, fileName);
    }
    
    // 创建索引
    index->spatialIndex = Index_Create(index->properties);
    if (!index->spatialIndex) {
        IndexProperty_Destroy(index->properties);
        free(index->fileName);
        free(index);
        return NULL;
    }
    
    index->isValid = Index_IsValid(index->spatialIndex) != 0;
    
    return index;
}

// 销毁三维R*树索引
void DestroyRStar3DIndex(RStar3DIndex* index) {
    if (!index) return;
    
    if (index->spatialIndex) {
        Index_Destroy(index->spatialIndex);
    }
    
    if (index->properties) {
        IndexProperty_Destroy(index->properties);
    }
    
    free(index->fileName);
    free(index);
}

// 检查索引是否有效
bool RStar3DIndex_IsValid(RStar3DIndex* index) {
    if (!index || !index->spatialIndex) return false;
    return index->isValid && (Index_IsValid(index->spatialIndex) != 0);
}

// 插入点数据
bool RStar3DIndex_InsertPoint(RStar3DIndex* index, const Point3D* point) {
    if (!index || !point || !RStar3DIndex_IsValid(index)) {
        return false;
    }
    
    double min[3] = {point->x, point->y, point->z};
    double max[3] = {point->x, point->y, point->z};
    
    RTError result = Index_InsertData(index->spatialIndex, point->id, min, max, 3,
                                     (const uint8_t*)point->userData, point->userDataSize);
    
    return result == RT_None;
}

// 插入边界框数据
bool RStar3DIndex_InsertBoundingBox(RStar3DIndex* index, int64_t id, const BoundingBox3D* bbox, 
                                   const void* userData, size_t userDataSize) {
    if (!index || !bbox || !RStar3DIndex_IsValid(index)) {
        return false;
    }
    
    double min[3] = {bbox->minX, bbox->minY, bbox->minZ};
    double max[3] = {bbox->maxX, bbox->maxY, bbox->maxZ};
    
    RTError result = Index_InsertData(index->spatialIndex, id, min, max, 3,
                                     (const uint8_t*)userData, userDataSize);
    
    return result == RT_None;
}

// 删除点数据
bool RStar3DIndex_DeletePoint(RStar3DIndex* index, const Point3D* point) {
    if (!index || !point || !RStar3DIndex_IsValid(index)) {
        return false;
    }
    
    double min[3] = {point->x, point->y, point->z};
    double max[3] = {point->x, point->y, point->z};
    
    RTError result = Index_DeleteData(index->spatialIndex, point->id, min, max, 3);
    
    return result == RT_None;
}

// 删除边界框数据
bool RStar3DIndex_DeleteBoundingBox(RStar3DIndex* index, int64_t id, const BoundingBox3D* bbox) {
    if (!index || !bbox || !RStar3DIndex_IsValid(index)) {
        return false;
    }
    
    double min[3] = {bbox->minX, bbox->minY, bbox->minZ};
    double max[3] = {bbox->maxX, bbox->maxY, bbox->maxZ};
    
    RTError result = Index_DeleteData(index->spatialIndex, id, min, max, 3);
    
    return result == RT_None;
}

// 相交查询
SpatialQueryResult3D* RStar3DIndex_IntersectionQuery(RStar3DIndex* index, const BoundingBox3D* queryBox) {
    if (!index || !queryBox || !RStar3DIndex_IsValid(index)) {
        return NULL;
    }
    
    double min[3] = {queryBox->minX, queryBox->minY, queryBox->minZ};
    double max[3] = {queryBox->maxX, queryBox->maxY, queryBox->maxZ};
    
    int64_t* ids;
    uint64_t nResults;
    
    RTError result = Index_Intersects_id(index->spatialIndex, min, max, 3, &ids, &nResults);
    
    if (result != RT_None || nResults == 0) {
        return NULL;
    }
    
    SpatialQueryResult3D* queryResult = CreateSpatialQueryResult3D();
    if (!queryResult) {
        Index_Free(ids);
        return NULL;
    }
    
    queryResult->ids = (int64_t*)malloc(nResults * sizeof(int64_t));
    if (!queryResult->ids) {
        DestroySpatialQueryResult3D(queryResult);
        Index_Free(ids);
        return NULL;
    }
    
    memcpy(queryResult->ids, ids, nResults * sizeof(int64_t));
    queryResult->count = (unsigned int)nResults;
    queryResult->capacity = (unsigned int)nResults;
    
    Index_Free(ids);
    return queryResult;
}

// 包含查询
SpatialQueryResult3D* RStar3DIndex_ContainmentQuery(RStar3DIndex* index, const BoundingBox3D* queryBox) {
    if (!index || !queryBox || !RStar3DIndex_IsValid(index)) {
        return NULL;
    }
    
    double min[3] = {queryBox->minX, queryBox->minY, queryBox->minZ};
    double max[3] = {queryBox->maxX, queryBox->maxY, queryBox->maxZ};
    
    int64_t* ids;
    uint64_t nResults;
    
    RTError result = Index_Contains_id(index->spatialIndex, min, max, 3, &ids, &nResults);
    
    if (result != RT_None || nResults == 0) {
        return NULL;
    }
    
    SpatialQueryResult3D* queryResult = CreateSpatialQueryResult3D();
    if (!queryResult) {
        Index_Free(ids);
        return NULL;
    }
    
    queryResult->ids = (int64_t*)malloc(nResults * sizeof(int64_t));
    if (!queryResult->ids) {
        DestroySpatialQueryResult3D(queryResult);
        Index_Free(ids);
        return NULL;
    }
    
    memcpy(queryResult->ids, ids, nResults * sizeof(int64_t));
    queryResult->count = (unsigned int)nResults;
    queryResult->capacity = (unsigned int)nResults;
    
    Index_Free(ids);
    return queryResult;
}

// 最近邻查询
SpatialQueryResult3D* RStar3DIndex_NearestNeighborQuery(RStar3DIndex* index, const Point3D* queryPoint, unsigned int k) {
    if (!index || !queryPoint || !RStar3DIndex_IsValid(index) || k == 0) {
        return NULL;
    }
    
    double min[3] = {queryPoint->x, queryPoint->y, queryPoint->z};
    double max[3] = {queryPoint->x, queryPoint->y, queryPoint->z};
    
    int64_t* ids;
    uint64_t nResults;
    
    RTError result = Index_NearestNeighbors_id(index->spatialIndex, min, max, 3, &ids, &nResults);
    
    if (result != RT_None || nResults == 0) {
        return NULL;
    }
    
    // 限制结果数量为k
    if (nResults > k) {
        nResults = k;
    }
    
    SpatialQueryResult3D* queryResult = CreateSpatialQueryResult3D();
    if (!queryResult) {
        Index_Free(ids);
        return NULL;
    }
    
    queryResult->ids = (int64_t*)malloc(nResults * sizeof(int64_t));
    if (!queryResult->ids) {
        DestroySpatialQueryResult3D(queryResult);
        Index_Free(ids);
        return NULL;
    }
    
    memcpy(queryResult->ids, ids, nResults * sizeof(int64_t));
    queryResult->count = (unsigned int)nResults;
    queryResult->capacity = (unsigned int)nResults;
    
    Index_Free(ids);
    return queryResult;
}

// 相交计数查询
unsigned int RStar3DIndex_IntersectionCount(RStar3DIndex* index, const BoundingBox3D* queryBox) {
    if (!index || !queryBox || !RStar3DIndex_IsValid(index)) {
        return 0;
    }
    
    double min[3] = {queryBox->minX, queryBox->minY, queryBox->minZ};
    double max[3] = {queryBox->maxX, queryBox->maxY, queryBox->maxZ};
    
    uint64_t nResults;
    RTError result = Index_Intersects_count(index->spatialIndex, min, max, 3, &nResults);
    
    if (result != RT_None) {
        return 0;
    }
    
    return (unsigned int)nResults;
}

// 获取索引边界
bool RStar3DIndex_GetBounds(RStar3DIndex* index, BoundingBox3D* bounds) {
    if (!index || !bounds || !RStar3DIndex_IsValid(index)) {
        return false;
    }
    
    double* pMins;
    double* pMaxs;
    uint32_t nDimension;
    
    RTError result = Index_GetBounds(index->spatialIndex, &pMins, &pMaxs, &nDimension);
    
    if (result != RT_None || nDimension != 3) {
        return false;
    }
    
    bounds->minX = pMins[0];
    bounds->minY = pMins[1];
    bounds->minZ = pMins[2];
    bounds->maxX = pMaxs[0];
    bounds->maxY = pMaxs[1];
    bounds->maxZ = pMaxs[2];
    
    Index_Free(pMins);
    Index_Free(pMaxs);
    
    return true;
}

// 刷新索引
void RStar3DIndex_Flush(RStar3DIndex* index) {
    if (index && index->spatialIndex) {
        Index_Flush(index->spatialIndex);
    }
}

// 清空缓冲区
void RStar3DIndex_ClearBuffer(RStar3DIndex* index) {
    if (index && index->spatialIndex) {
        Index_ClearBuffer(index->spatialIndex);
    }
}

// 辅助函数实现

// 创建三维点
Point3D* CreatePoint3D(double x, double y, double z, int64_t id, const void* userData, size_t userDataSize) {
    Point3D* point = (Point3D*)malloc(sizeof(Point3D));
    if (!point) return NULL;
    
    point->x = x;
    point->y = y;
    point->z = z;
    point->id = id;
    point->userData = NULL;
    point->userDataSize = userDataSize;
    
    if (userData && userDataSize > 0) {
        point->userData = malloc(userDataSize);
        if (point->userData) {
            memcpy(point->userData, userData, userDataSize);
        } else {
            point->userDataSize = 0;
        }
    }
    
    return point;
}

// 销毁三维点
void DestroyPoint3D(Point3D* point) {
    if (point) {
        free(point->userData);
        free(point);
    }
}

// 创建边界框
BoundingBox3D* CreateBoundingBox3D(double minX, double minY, double minZ, 
                                   double maxX, double maxY, double maxZ) {
    BoundingBox3D* bbox = (BoundingBox3D*)malloc(sizeof(BoundingBox3D));
    if (!bbox) return NULL;
    
    bbox->minX = minX;
    bbox->minY = minY;
    bbox->minZ = minZ;
    bbox->maxX = maxX;
    bbox->maxY = maxY;
    bbox->maxZ = maxZ;
    
    return bbox;
}

// 销毁边界框
void DestroyBoundingBox3D(BoundingBox3D* bbox) {
    free(bbox);
}

// 创建查询结果
SpatialQueryResult3D* CreateSpatialQueryResult3D() {
    SpatialQueryResult3D* result = (SpatialQueryResult3D*)malloc(sizeof(SpatialQueryResult3D));
    if (!result) return NULL;
    
    result->ids = NULL;
    result->points = NULL;
    result->count = 0;
    result->capacity = 0;
    
    return result;
}

// 销毁查询结果
void DestroySpatialQueryResult3D(SpatialQueryResult3D* result) {
    if (result) {
        free(result->ids);
        if (result->points) {
            for (unsigned int i = 0; i < result->count; i++) {
                free(result->points[i].userData);
            }
            free(result->points);
        }
        free(result);
    }
}

// 实用函数实现

// 检查两个边界框是否相交
bool BoundingBox3D_Intersects(const BoundingBox3D* box1, const BoundingBox3D* box2) {
    if (!box1 || !box2) return false;
    
    return !(box1->maxX < box2->minX || box1->minX > box2->maxX ||
             box1->maxY < box2->minY || box1->minY > box2->maxY ||
             box1->maxZ < box2->minZ || box1->minZ > box2->maxZ);
}

// 检查一个边界框是否包含另一个边界框
bool BoundingBox3D_Contains(const BoundingBox3D* container, const BoundingBox3D* contained) {
    if (!container || !contained) return false;
    
    return (container->minX <= contained->minX && container->maxX >= contained->maxX &&
            container->minY <= contained->minY && container->maxY >= contained->maxY &&
            container->minZ <= contained->minZ && container->maxZ >= contained->maxZ);
}

// 检查边界框是否包含点
bool BoundingBox3D_ContainsPoint(const BoundingBox3D* box, const Point3D* point) {
    if (!box || !point) return false;
    
    return (point->x >= box->minX && point->x <= box->maxX &&
            point->y >= box->minY && point->y <= box->maxY &&
            point->z >= box->minZ && point->z <= box->maxZ);
}

// 计算两点之间的距离
double Point3D_Distance(const Point3D* p1, const Point3D* p2) {
    if (!p1 || !p2) return -1.0;
    
    double dx = p1->x - p2->x;
    double dy = p1->y - p2->y;
    double dz = p1->z - p2->z;
    
    return sqrt(dx*dx + dy*dy + dz*dz);
}

// 扩展边界框以包含点
void BoundingBox3D_Expand(BoundingBox3D* box, const Point3D* point) {
    if (!box || !point) return;
    
    if (point->x < box->minX) box->minX = point->x;
    if (point->x > box->maxX) box->maxX = point->x;
    if (point->y < box->minY) box->minY = point->y;
    if (point->y > box->maxY) box->maxY = point->y;
    if (point->z < box->minZ) box->minZ = point->z;
    if (point->z > box->maxZ) box->maxZ = point->z;
} 