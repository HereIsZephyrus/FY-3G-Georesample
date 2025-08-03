#include "test_suites.h"
#include <float.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <stdlib.h>
#include "rstartree.h"

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
    
    double queryPoint[3] = {1.0, 2.0, 3.0};
    SpatialQueryResult* queryResult = RStarIndex_NearestNeighborQuery(index, queryPoint, 2);
    TEST_ASSERT_NOT_NULL(queryResult);
    TEST_ASSERT_TRUE(queryResult->count >= 2);
    
    bool found_id1 = false, found_id2 = false;
    for (unsigned int i = 0; i < queryResult->count; i++) {
        if (queryResult->ids[i] == 1) found_id1 = true;
        if (queryResult->ids[i] == 2) found_id2 = true;
    }
    TEST_ASSERT_TRUE(found_id1);
    TEST_ASSERT_TRUE(found_id2);
    
    TEST_MESSAGE("RStar3D basic functionality test completed");
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
    
    double queryPoint[3] = {1.5, 1.5, 1.5};
    
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

    for (int i = 0; i < 4; i++)
        DestroyRStarPoint(rstarPoints[i]);
    DestroyRStarIndex(index);
    
    TEST_MESSAGE("RStar3D nearest neighbor test completed");
}

void test_rstar3d_large_scale_insertion(void) {
    TEST_MESSAGE("Start RStar3D large scale insertion test");
    
    RStarIndex* index = CreateRStarIndex(100, 0.7);
    TEST_ASSERT_NOT_NULL(index);
    
    const unsigned int NUM_POINTS = 10000;
    RStarPoint** points = (RStarPoint**)malloc(NUM_POINTS * sizeof(RStarPoint*));
    TEST_ASSERT_NOT_NULL(points);
    
    for (unsigned int i = 0; i < NUM_POINTS; i++) {
        float lat = (float)(rand() % 1000) / 10.0f;
        float lon = (float)(rand() % 1000) / 10.0f;
        float height = (float)(rand() % 500) / 10.0f;
        
        points[i] = CreateRStarPoint(lat, lon, height, i + 1, NULL, 0);
        TEST_ASSERT_NOT_NULL(points[i]);
        
        bool result = RStarIndex_InsertPoint(index, points[i]);
        TEST_ASSERT_TRUE(result);
    }
    
    double queryPoint[3] = {25.0, 25.0, 10.0};
    SpatialQueryResult* queryResult = RStarIndex_NearestNeighborQuery(index, queryPoint, 1000);
    TEST_ASSERT_NOT_NULL(queryResult);
    
    TEST_ASSERT_TRUE(queryResult->count > 0);
    TEST_ASSERT_TRUE(queryResult->count < NUM_POINTS);
    
    DestroySpatialQueryResult(queryResult);
    for (unsigned int i = 0; i < NUM_POINTS; i++) {
        DestroyRStarPoint(points[i]);
    }
    free(points);
    DestroyRStarIndex(index);
    
    TEST_MESSAGE("RStar3D large scale insertion test completed");
}

void test_rstar3d_complex_spatial_queries(void) {
    TEST_MESSAGE("Start RStar3D complex spatial queries test");
    
    RStarIndex* index = CreateRStarIndex(50, 0.8);
    TEST_ASSERT_NOT_NULL(index);
    
    const int CLUSTERS = 5;
    const int POINTS_PER_CLUSTER = 100;
    RStarPoint** points = (RStarPoint**)malloc(CLUSTERS * POINTS_PER_CLUSTER * sizeof(RStarPoint*));
    
    int pointId = 1;
    for (int cluster = 0; cluster < CLUSTERS; cluster++) {
        float centerLat = 20.0f + cluster * 15.0f;
        float centerLon = 20.0f + cluster * 15.0f;
        float centerHeight = 10.0f + cluster * 5.0f;
        
        for (int i = 0; i < POINTS_PER_CLUSTER; i++) {
            float lat = centerLat + ((float)(rand() % 100) / 50.0f - 1.0f) * 5.0f;
            float lon = centerLon + ((float)(rand() % 100) / 50.0f - 1.0f) * 5.0f;
            float height = centerHeight + ((float)(rand() % 100) / 50.0f - 1.0f) * 2.0f;
            
            points[(cluster * POINTS_PER_CLUSTER) + i] = CreateRStarPoint(lat, lon, height, pointId++, NULL, 0);
            bool result = RStarIndex_InsertPoint(index, points[(cluster * POINTS_PER_CLUSTER) + i]);
            TEST_ASSERT_TRUE(result);
        }
    }
    
    double smallQuery[3] = {18.0, 18.0, 8.0};
    double mediumQuery[3] = {15.0, 15.0, 5.0};
    double largeQuery[3] = {0.0, 0.0, 0.0};
    
    SpatialQueryResult* smallResult = RStarIndex_NearestNeighborQuery(index, smallQuery, 1000);
    SpatialQueryResult* mediumResult = RStarIndex_NearestNeighborQuery(index, mediumQuery, 1000);
    SpatialQueryResult* largeResult = RStarIndex_NearestNeighborQuery(index, largeQuery, 1000);
    
    TEST_ASSERT_NOT_NULL(smallResult);
    TEST_ASSERT_NOT_NULL(mediumResult);
    TEST_ASSERT_NOT_NULL(largeResult);
    TEST_ASSERT_TRUE(smallResult->count <= mediumResult->count);
    TEST_ASSERT_TRUE(mediumResult->count <= largeResult->count);
    
    // check count of query result
    unsigned int smallCount = RStarIndex_NearestNeighborQuery(index, smallQuery, 1000)->count;
    unsigned int mediumCount = RStarIndex_NearestNeighborQuery(index, mediumQuery, 1000)->count;
    unsigned int largeCount = RStarIndex_NearestNeighborQuery(index, largeQuery, 1000)->count;
    
    TEST_ASSERT_EQUAL_INT(smallResult->count, smallCount);
    TEST_ASSERT_EQUAL_INT(mediumResult->count, mediumCount);
    TEST_ASSERT_EQUAL_INT(largeResult->count, largeCount);
    
    DestroySpatialQueryResult(smallResult);
    DestroySpatialQueryResult(mediumResult);
    DestroySpatialQueryResult(largeResult);
    
    for (int i = 0; i < CLUSTERS * POINTS_PER_CLUSTER; i++) {
        DestroyRStarPoint(points[i]);
    }
    free(points);
    DestroyRStarIndex(index);
    
    TEST_MESSAGE("RStar3D complex spatial queries test completed");
}

void test_rstar3d_mixed_operations_stress(void) {
    TEST_MESSAGE("Start RStar3D mixed operations stress test");
    
    RStarIndex* index = CreateRStarIndex(200, 0.75);
    TEST_ASSERT_NOT_NULL(index);
    
    const int OPERATIONS = 5000;
    RStarPoint** points = (RStarPoint**)malloc(OPERATIONS * sizeof(RStarPoint*));
    
    int insertedPoints = 0;
    
    for (int op = 0; op < OPERATIONS; op++) {
        int operation = rand() % 100;
        
        if (operation < 70) {  // 70% insert at random position
            float lat = (float)(rand() % 2000) / 20.0f;
            float lon = (float)(rand() % 2000) / 20.0f;
            float height = (float)(rand() % 1000) / 20.0f;
            
            points[insertedPoints] = CreateRStarPoint(lat, lon, height, insertedPoints + 1, NULL, 0);
            bool result = RStarIndex_InsertPoint(index, points[insertedPoints]);
            TEST_ASSERT_TRUE(result);
            insertedPoints++;
            
        } else if (operation < 90) {  // 20% query
            float queryLat = (float)(rand() % 1000) / 10.0f;
            float queryLon = (float)(rand() % 1000) / 10.0f;
            float queryHeight = (float)(rand() % 500) / 10.0f;
            
            double queryPoint[3] = {queryLat, queryLon, queryHeight};
            SpatialQueryResult* result = RStarIndex_NearestNeighborQuery(index, queryPoint, 1000);
            if (result)
                DestroySpatialQueryResult(result);
            
        } else {  // 10% delete
            if (insertedPoints > 0 && rand() % 2 == 0) {
                int deleteIdx = rand() % insertedPoints;
                if (points[deleteIdx] != NULL) {
                    RStarIndex_DeletePoint(index, points[deleteIdx]);
                    DestroyRStarPoint(points[deleteIdx]);
                    points[deleteIdx] = NULL;
                }
            }
        }
    }
    
    TEST_ASSERT_TRUE(IsRStarIndexValid(index));
    
    for (int i = 0; i < insertedPoints; i++) {
        if (points[i] != NULL) {
            DestroyRStarPoint(points[i]);
        }
    }
    free(points);
    DestroyRStarIndex(index);
    
    TEST_MESSAGE("RStar3D mixed operations stress test completed");
}

void test_rstar3d_nearest_neighbor_advanced(void) {
    TEST_MESSAGE("Start RStar3D advanced nearest neighbor test");
    
    RStarIndex* index = CreateRStarIndex(100, 0.7);
    TEST_ASSERT_NOT_NULL(index);
    
    const int GRID_SIZE = 20;
    const float SPACING = 5.0f;
    RStarPoint** gridPoints = (RStarPoint**)malloc(GRID_SIZE * GRID_SIZE * GRID_SIZE * sizeof(RStarPoint*));
    
    int pointCount = 0;
    for (int x = 0; x < GRID_SIZE; x++) {
        for (int y = 0; y < GRID_SIZE; y++) {
            for (int z = 0; z < GRID_SIZE; z++) {
                float lat = x * SPACING;
                float lon = y * SPACING;
                float height = z * SPACING;
                
                gridPoints[pointCount] = CreateRStarPoint(lat, lon, height, pointCount + 1, NULL, 0);
                bool result = RStarIndex_InsertPoint(index, gridPoints[pointCount]);
                TEST_ASSERT_TRUE(result);
                pointCount++;
            }
        }
    }
    
    double queryPoint[3] = {10.5, 15.5, 12.5};
    
    for (unsigned int k = 1; k <= 27; k += 5) {
        SpatialQueryResult* nnResult = RStarIndex_NearestNeighborQuery(index, queryPoint, k);
        
        if (nnResult) {
            TEST_ASSERT_TRUE(nnResult->count > 0);
            TEST_ASSERT_TRUE(nnResult->count <= k);
            DestroySpatialQueryResult(nnResult);
        }
    }
    
    for (int i = 0; i < pointCount; i++)
        DestroyRStarPoint(gridPoints[i]);
    free(gridPoints);
    DestroyRStarIndex(index);
    
    TEST_MESSAGE("RStar3D advanced nearest neighbor test completed");
}

void test_rstar3d_boundary_conditions(void) {
    TEST_MESSAGE("Start RStar3D boundary conditions test");
    
    RStarIndex* index = CreateRStarIndex(100, 0.5);
    TEST_ASSERT_NOT_NULL(index);
    
    RStarPoint* minPoint = CreateRStarPoint(-1000.0, -1000.0, -1000.0, 1, NULL, 0);
    RStarPoint* maxPoint = CreateRStarPoint(1000.0, 1000.0, 1000.0, 2, NULL, 0);
    RStarPoint* zeroPoint = CreateRStarPoint(0.0, 0.0, 0.0, 3, NULL, 0);
    
    TEST_ASSERT_TRUE(RStarIndex_InsertPoint(index, minPoint));
    TEST_ASSERT_TRUE(RStarIndex_InsertPoint(index, maxPoint));
    TEST_ASSERT_TRUE(RStarIndex_InsertPoint(index, zeroPoint));
    
    RStarPoint* duplicatePoint = CreateRStarPoint(0.0, 0.0, 0.0, 4, NULL, 0);
    TEST_ASSERT_TRUE(RStarIndex_InsertPoint(index, duplicatePoint));
    
    double emptyQuery[3] = {500.0, 500.0, 500.0};
    SpatialQueryResult* emptyResult = RStarIndex_NearestNeighborQuery(index, emptyQuery, 1000);
    if (emptyResult) {
        DestroySpatialQueryResult(emptyResult);
    }
    
    double hugeQuery[3] = {-2000.0, -2000.0, -2000.0};
    SpatialQueryResult* hugeResult = RStarIndex_NearestNeighborQuery(index, hugeQuery, 1000);
    TEST_ASSERT_NOT_NULL(hugeResult);
    TEST_ASSERT_EQUAL_INT(4, hugeResult->count);
    
    double invalidQuery[3] = {10.0, 10.0, 10.0};
    SpatialQueryResult* invalidResult = RStarIndex_NearestNeighborQuery(index, invalidQuery, 1000);
    if (invalidResult) {
        DestroySpatialQueryResult(invalidResult);
    }
    
    DestroySpatialQueryResult(hugeResult);
    DestroyRStarPoint(minPoint);
    DestroyRStarPoint(maxPoint);
    DestroyRStarPoint(zeroPoint);
    DestroyRStarPoint(duplicatePoint);
    DestroyRStarIndex(index);
    
    TEST_MESSAGE("RStar3D boundary conditions test completed");
}

void test_rstar3d(void) {
    TEST_MESSAGE("Start RStar3D test");
    RUN_TEST(test_rstar3d_basic_functionality);
    RUN_TEST(test_rstar3d_nearest_neighbor);
    RUN_TEST(test_rstar3d_large_scale_insertion);
    RUN_TEST(test_rstar3d_complex_spatial_queries);
    RUN_TEST(test_rstar3d_mixed_operations_stress);
    RUN_TEST(test_rstar3d_nearest_neighbor_advanced);
    RUN_TEST(test_rstar3d_boundary_conditions);
    TEST_MESSAGE("RStar3D test completed");
}
