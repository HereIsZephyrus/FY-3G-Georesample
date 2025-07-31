#include "test_suites.h"
#include "../include/index.h"
#include <float.h>
#include <string.h>

// 基础功能测试 - 简化版本，专注于测试R树功能

void test_rstar3d_basic_functionality(void) {
    printf("测试三维R*树基础功能...\n");
    
    // 这里我们先测试基本的libspatialindex C API功能
    // 创建索引属性
    IndexPropertyH props = IndexProperty_Create();
    TEST_ASSERT_NOT_NULL(props);
    
    // 设置为3维R*树
    IndexProperty_SetIndexType(props, RT_RTree);
    IndexProperty_SetIndexVariant(props, RT_Star);
    IndexProperty_SetDimension(props, 3);
    IndexProperty_SetIndexStorage(props, RT_Memory);
    IndexProperty_SetIndexCapacity(props, 50);
    IndexProperty_SetLeafCapacity(props, 50);
    IndexProperty_SetFillFactor(props, 0.7);
    
    // 创建索引
    IndexH index = Index_Create(props);
    TEST_ASSERT_NOT_NULL(index);
    TEST_ASSERT_TRUE(Index_IsValid(index));
    
    // 插入一些3D点
    double min1[3] = {1.0, 2.0, 3.0};
    double max1[3] = {1.0, 2.0, 3.0};
    RTError result1 = Index_InsertData(index, 1, min1, max1, 3, NULL, 0);
    TEST_ASSERT_EQUAL_INT(RT_None, result1);
    
    double min2[3] = {4.0, 5.0, 6.0};
    double max2[3] = {4.0, 5.0, 6.0};
    RTError result2 = Index_InsertData(index, 2, min2, max2, 3, NULL, 0);
    TEST_ASSERT_EQUAL_INT(RT_None, result2);
    
    double min3[3] = {7.0, 8.0, 9.0};
    double max3[3] = {7.0, 8.0, 9.0};
    RTError result3 = Index_InsertData(index, 3, min3, max3, 3, NULL, 0);
    TEST_ASSERT_EQUAL_INT(RT_None, result3);
    
    // 执行相交查询
    double queryMin[3] = {0.0, 0.0, 0.0};
    double queryMax[3] = {5.0, 5.0, 5.0};
    int64_t* ids;
    uint64_t nResults;
    
    RTError queryResult = Index_Intersects_id(index, queryMin, queryMax, 3, &ids, &nResults);
    TEST_ASSERT_EQUAL_INT(RT_None, queryResult);
    TEST_ASSERT_TRUE(nResults >= 2);  // 应该找到至少2个点
    
    // 验证结果
    bool found_id1 = false, found_id2 = false;
    for (uint64_t i = 0; i < nResults; i++) {
        if (ids[i] == 1) found_id1 = true;
        if (ids[i] == 2) found_id2 = true;
    }
    TEST_ASSERT_TRUE(found_id1);
    TEST_ASSERT_TRUE(found_id2);
    
    // 执行计数查询
    uint64_t count;
    RTError countResult = Index_Intersects_count(index, queryMin, queryMax, 3, &count);
    TEST_ASSERT_EQUAL_INT(RT_None, countResult);
    TEST_ASSERT_EQUAL_INT(nResults, count);
    
    // 获取索引边界
    double* pMins;
    double* pMaxs;
    uint32_t nDimension;
    RTError boundsResult = Index_GetBounds(index, &pMins, &pMaxs, &nDimension);
    TEST_ASSERT_EQUAL_INT(RT_None, boundsResult);
    TEST_ASSERT_EQUAL_INT(3, nDimension);
    TEST_ASSERT_TRUE(pMins[0] <= 1.0);
    TEST_ASSERT_TRUE(pMaxs[0] >= 7.0);
    TEST_ASSERT_TRUE(pMins[1] <= 2.0);
    TEST_ASSERT_TRUE(pMaxs[1] >= 8.0);
    TEST_ASSERT_TRUE(pMins[2] <= 3.0);
    TEST_ASSERT_TRUE(pMaxs[2] >= 9.0);
    
    // 清理资源
    Index_Free(ids);
    Index_Free(pMins);
    Index_Free(pMaxs);
    Index_Destroy(index);
    IndexProperty_Destroy(props);
    
    printf("三维R*树基础功能测试完成!\n");
}

void test_rstar3d_bounding_box_operations(void) {
    printf("测试三维边界框操作...\n");
    
    // 创建索引
    IndexPropertyH props = IndexProperty_Create();
    IndexProperty_SetIndexType(props, RT_RTree);
    IndexProperty_SetIndexVariant(props, RT_Star);
    IndexProperty_SetDimension(props, 3);
    IndexProperty_SetIndexStorage(props, RT_Memory);
    IndexProperty_SetIndexCapacity(props, 50);
    IndexProperty_SetLeafCapacity(props, 50);
    IndexProperty_SetFillFactor(props, 0.7);
    
    IndexH index = Index_Create(props);
    TEST_ASSERT_NOT_NULL(index);
    
    // 插入3D边界框
    double min1[3] = {0.0, 0.0, 0.0};
    double max1[3] = {2.0, 2.0, 2.0};
    const char* data1 = "边界框1";
    RTError result1 = Index_InsertData(index, 100, min1, max1, 3, 
                                      (const uint8_t*)data1, strlen(data1) + 1);
    TEST_ASSERT_EQUAL_INT(RT_None, result1);
    
    double min2[3] = {5.0, 5.0, 5.0};
    double max2[3] = {7.0, 7.0, 7.0};
    const char* data2 = "边界框2";
    RTError result2 = Index_InsertData(index, 101, min2, max2, 3, 
                                      (const uint8_t*)data2, strlen(data2) + 1);
    TEST_ASSERT_EQUAL_INT(RT_None, result2);
    
    // 相交查询 - 应该与第一个边界框相交
    double queryMin[3] = {1.0, 1.0, 1.0};
    double queryMax[3] = {3.0, 3.0, 3.0};
    int64_t* ids;
    uint64_t nResults;
    
    RTError queryResult = Index_Intersects_id(index, queryMin, queryMax, 3, &ids, &nResults);
    TEST_ASSERT_EQUAL_INT(RT_None, queryResult);
    TEST_ASSERT_TRUE(nResults >= 1);
    
    bool found_bbox1 = false;
    for (uint64_t i = 0; i < nResults; i++) {
        if (ids[i] == 100) found_bbox1 = true;
    }
    TEST_ASSERT_TRUE(found_bbox1);
    
    // 删除边界框
    RTError deleteResult = Index_DeleteData(index, 100, min1, max1, 3);
    TEST_ASSERT_EQUAL_INT(RT_None, deleteResult);
    
    // 再次查询验证删除
    Index_Free(ids);
    RTError queryResult2 = Index_Intersects_id(index, queryMin, queryMax, 3, &ids, &nResults);
    TEST_ASSERT_EQUAL_INT(RT_None, queryResult2);
    
    bool found_bbox1_after_delete = false;
    for (uint64_t i = 0; i < nResults; i++) {
        if (ids[i] == 100) found_bbox1_after_delete = true;
    }
    TEST_ASSERT_FALSE(found_bbox1_after_delete);
    
    // 清理资源
    Index_Free(ids);
    Index_Destroy(index);
    IndexProperty_Destroy(props);
    
    printf("三维边界框操作测试完成!\n");
}

void test_rstar3d_nearest_neighbor(void) {
    printf("测试三维最近邻查询...\n");
    
    // 创建索引
    IndexPropertyH props = IndexProperty_Create();
    IndexProperty_SetIndexType(props, RT_RTree);
    IndexProperty_SetIndexVariant(props, RT_Star);
    IndexProperty_SetDimension(props, 3);
    IndexProperty_SetIndexStorage(props, RT_Memory);
    
    IndexH index = Index_Create(props);
    TEST_ASSERT_NOT_NULL(index);
    
    // 插入多个点
    double points[][3] = {
        {1.0, 1.0, 1.0},   // ID: 1
        {2.0, 2.0, 2.0},   // ID: 2
        {5.0, 5.0, 5.0},   // ID: 3
        {10.0, 10.0, 10.0} // ID: 4
    };
    
    for (int i = 0; i < 4; i++) {
        RTError result = Index_InsertData(index, i + 1, points[i], points[i], 3, NULL, 0);
        TEST_ASSERT_EQUAL_INT(RT_None, result);
    }
    
    // 最近邻查询 - 查找距离(1.5, 1.5, 1.5)最近的点
    double queryPoint[3] = {1.5, 1.5, 1.5};
    int64_t* ids;
    uint64_t nResults;
    
    RTError nnResult = Index_NearestNeighbors_id(index, queryPoint, queryPoint, 3, &ids, &nResults);
    TEST_ASSERT_EQUAL_INT(RT_None, nnResult);
    TEST_ASSERT_TRUE(nResults > 0);
    
    // 最近的点应该是ID为1或2的点
    bool found_close_point = false;
    for (uint64_t i = 0; i < nResults; i++) {
        if (ids[i] == 1 || ids[i] == 2) {
            found_close_point = true;
            break;
        }
    }
    TEST_ASSERT_TRUE(found_close_point);
    
    // 清理资源
    Index_Free(ids);
    Index_Destroy(index);
    IndexProperty_Destroy(props);
    
    printf("三维最近邻查询测试完成!\n");
}

// 主测试函数
void test_rstar3d(void) {
    printf("开始三维R*树测试...\n");
    
    RUN_TEST(test_rstar3d_basic_functionality);
    RUN_TEST(test_rstar3d_bounding_box_operations);
    RUN_TEST(test_rstar3d_nearest_neighbor);
    
    printf("三维R*树测试完成!\n");
}
