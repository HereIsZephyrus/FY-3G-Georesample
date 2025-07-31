#include "test_suites.h"
#include <float.h>
#include <string.h>
#include "index.h"


void test_rstar3d_basic_functionality(void) {
    TEST_MESSAGE("Start RStar3D basic functionality test");
    
    RStarIndex* index = CreateRStarIndex(50, 0.7);
    TEST_ASSERT_NOT_NULL(index);
    TEST_ASSERT_TRUE(IsRStarIndexValid(index));
    
    RStarPoint* point1 = CreateRStarPoint(1.0, 2.0, 3.0, 1, NULL, 0);
    TEST_ASSERT_NOT_NULL(point1);
    bool result1 = RStarIndex_InsertPoint(index, point1);
    TEST_ASSERT_TRUE(result1);
    
    RStarPoint* point2 = CreateRStarPoint(4.0, 4.9, 4.9, 2, NULL, 0);
    TEST_ASSERT_NOT_NULL(point2);
    bool result2 = RStarIndex_InsertPoint(index, point2);
    TEST_ASSERT_TRUE(result2);
    
    RStarPoint* point3 = CreateRStarPoint(7.0, 8.0, 9.0, 3, NULL, 0);
    TEST_ASSERT_NOT_NULL(point3);
    bool result3 = RStarIndex_InsertPoint(index, point3);
    TEST_ASSERT_TRUE(result3);
    
    BoundingBox* queryBox = CreateBoundingBox(0.0, 0.0, 0.0, 5.0, 5.0, 5.0);
    TEST_ASSERT_NOT_NULL(queryBox);
    
    SpatialQueryResult* queryResult = RStarIndex_IntersectionQuery(index, queryBox);
    TEST_ASSERT_NOT_NULL(queryResult);
    TEST_ASSERT_TRUE(queryResult->count >= 2);
    
    bool found_id1 = false, found_id2 = false;
    for (unsigned int i = 0; i < queryResult->count; i++) {
        if (queryResult->ids[i] == 1) found_id1 = true;
        if (queryResult->ids[i] == 2) found_id2 = true;
    }
    TEST_ASSERT_TRUE(found_id1);
    TEST_ASSERT_TRUE(found_id2);
    
    unsigned int count = RStarIndex_IntersectionCount(index, queryBox);
    TEST_ASSERT_EQUAL_INT(queryResult->count, count);
    
    BoundingBox bounds;
    bool boundsResult = RStarIndex_GetBounds(index, &bounds);
    TEST_ASSERT_TRUE(boundsResult);
    TEST_ASSERT_TRUE(bounds.minLatitude <= 1.0);
    TEST_ASSERT_TRUE(bounds.maxLatitude >= 7.0);
    TEST_ASSERT_TRUE(bounds.minLongitude <= 2.0);
    TEST_ASSERT_TRUE(bounds.maxLongitude >= 8.0);
    TEST_ASSERT_TRUE(bounds.minHeight <= 3.0);
    TEST_ASSERT_TRUE(bounds.maxHeight >= 9.0);
    
    DestroySpatialQueryResult(queryResult);
    DestroyBoundingBox(queryBox);
    DestroyRStarPoint(point1);
    DestroyRStarPoint(point2);
    DestroyRStarPoint(point3);
    DestroyRStarIndex(index);
    
    TEST_MESSAGE("RStar3D basic functionality test completed");
}

void test_rstar3d_bounding_box_operations(void) {
    TEST_MESSAGE("Start RStar3D bounding box operations test");
    
    RStarIndex* index = CreateRStarIndex(50, 0.7);
    TEST_ASSERT_NOT_NULL(index);
    
    BoundingBox* bbox1 = CreateBoundingBox(0.0, 0.0, 0.0, 2.0, 2.0, 2.0);
    TEST_ASSERT_NOT_NULL(bbox1);
    const char* data1 = "边界框1";
    bool result1 = RStarIndex_InsertBoundingBox(index, 100, bbox1, data1, strlen(data1) + 1);
    TEST_ASSERT_TRUE(result1);
    
    BoundingBox* bbox2 = CreateBoundingBox(5.0, 5.0, 5.0, 7.0, 7.0, 7.0);
    TEST_ASSERT_NOT_NULL(bbox2);
    const char* data2 = "边界框2";
    bool result2 = RStarIndex_InsertBoundingBox(index, 101, bbox2, data2, strlen(data2) + 1);
    TEST_ASSERT_TRUE(result2);
    
    BoundingBox* queryBox = CreateBoundingBox(1.0, 1.0, 1.0, 3.0, 3.0, 3.0);
    TEST_ASSERT_NOT_NULL(queryBox);
    
    SpatialQueryResult* queryResult = RStarIndex_IntersectionQuery(index, queryBox);
    TEST_ASSERT_NOT_NULL(queryResult);
    TEST_ASSERT_TRUE(queryResult->count >= 1);
    
    bool found_bbox1 = false;
    for (unsigned int i = 0; i < queryResult->count; i++) {
        if (queryResult->ids[i] == 100) found_bbox1 = true;
    }
    TEST_ASSERT_TRUE(found_bbox1);
    
    bool deleteResult = RStarIndex_DeleteBoundingBox(index, 100, bbox1);
    TEST_ASSERT_TRUE(deleteResult);
    
    DestroySpatialQueryResult(queryResult);
    queryResult = RStarIndex_IntersectionQuery(index, queryBox);
    
    bool found_bbox1_after_delete = false;
    if (queryResult) {
        for (unsigned int i = 0; i < queryResult->count; i++) {
            if (queryResult->ids[i] == 100) found_bbox1_after_delete = true;
        }
    }
    TEST_ASSERT_FALSE(found_bbox1_after_delete);
    
    if (queryResult) DestroySpatialQueryResult(queryResult);
    DestroyBoundingBox(queryBox);
    DestroyBoundingBox(bbox1);
    DestroyBoundingBox(bbox2);
    DestroyRStarIndex(index);
    
    TEST_MESSAGE("RStar3D bounding box operations test completed");
}

void test_rstar3d_nearest_neighbor(void) {
    TEST_MESSAGE("Start RStar3D nearest neighbor test");
    
    RStarIndex* index = CreateRStarIndex(50, 0.7);
    TEST_ASSERT_NOT_NULL(index);
    
    float points[][3] = {
        {1.0, 1.0, 1.0},   // ID: 1
        {2.0, 2.0, 2.0},   // ID: 2
        {5.0, 5.0, 5.0},   // ID: 3
        {10.0, 10.0, 10.0} // ID: 4
    };
    
    RStarPoint* rstarPoints[4];
    for (int i = 0; i < 4; i++) {
        rstarPoints[i] = CreateRStarPoint(points[i][0], points[i][1], points[i][2], i + 1, NULL, 0);
        TEST_ASSERT_NOT_NULL(rstarPoints[i]);
        bool result = RStarIndex_InsertPoint(index, rstarPoints[i]);
        TEST_ASSERT_TRUE(result);
    }
    
    RStarPoint* queryPoint = CreateRStarPoint(1.5, 1.5, 1.5, 0, NULL, 0);
    TEST_ASSERT_NOT_NULL(queryPoint);
    
    SpatialQueryResult* nnResult = RStarIndex_NearestNeighborQuery(index, queryPoint, 4);
    TEST_ASSERT_NOT_NULL(nnResult);
    TEST_ASSERT_TRUE(nnResult->count > 0);
    
    bool found_close_point = false;
    for (unsigned int i = 0; i < nnResult->count; i++) {
        if (nnResult->ids[i] == 1 || nnResult->ids[i] == 2) {
            found_close_point = true;
            break;
        }
    }
    TEST_ASSERT_TRUE(found_close_point);
    
    DestroySpatialQueryResult(nnResult);
    DestroyRStarPoint(queryPoint);
    for (int i = 0; i < 4; i++)
        DestroyRStarPoint(rstarPoints[i]);
    DestroyRStarIndex(index);
    
    TEST_MESSAGE("RStar3D nearest neighbor test completed");
}

void test_rstar3d(void) {
    TEST_MESSAGE("Start RStar3D test");
    RUN_TEST(test_rstar3d_basic_functionality);
    RUN_TEST(test_rstar3d_bounding_box_operations);
    RUN_TEST(test_rstar3d_nearest_neighbor);
    TEST_MESSAGE("RStar3D test completed");
}
