#include "test_suites.h"
#include "kdtree.h"
#include "index.h"
#include <float.h>
#include <math.h>
#include <string.h>

static KDCalcPoint* CreateTestKDCalcPoints(int count) {
    KDCalcPoint* points = (KDCalcPoint*)malloc(count * sizeof(KDCalcPoint));
    if (!points) return NULL;
    
    for (int i = 0; i < count; i++) {
        points[i].latitude = (float)(i * 10.0 + 5.0);  // 5.0, 15.0, 25.0, ...
        points[i].longitude = (float)(i * 5.0 + 2.0);  // 2.0, 7.0, 12.0, ...
        points[i].id = i + 1;
        points[i].lat_sum = points[i].latitude * (i + 1);
        points[i].lon_sum = points[i].longitude * (i + 1);
        points[i].lat_square_sum = points[i].latitude * points[i].latitude * (i + 1);
        points[i].lon_square_sum = points[i].longitude * points[i].longitude * (i + 1);
    }
    return points;
}

static bool ValidateKDNode(const KDNode* node, int depth) {
    if (!node) return true;
    
    if (node->split_dim != 0 && node->split_dim != 1) return false;
    
    if (!ValidateKDNode(node->left, depth + 1)) return false;
    if (!ValidateKDNode(node->right, depth + 1)) return false;
    
    if (node->left) {
        if (node->split_dim == 0) {
            if (node->left->latitude > node->latitude) return false;
        } else {
            if (node->left->longitude > node->longitude) return false;
        }
    }
    
    if (node->right) {
        if (node->split_dim == 0) {
            if (node->right->latitude < node->latitude) return false;
        } else {
            if (node->right->longitude < node->longitude) return false;
        }
    }
    
    return true;
}

void test_kdtree_select_split_dimension(void) {
    TEST_MESSAGE("Start KDTree select split dimension test");
    
    int result = SelectSplitDimension(NULL, 0);
    TEST_ASSERT_EQUAL_INT(0, result);
    
    KDCalcPoint* points = CreateTestKDCalcPoints(1);
    TEST_ASSERT_NOT_NULL(points);
    result = SelectSplitDimension(points, 1);
    TEST_ASSERT_EQUAL_INT(0, result);
    
    free(points);
    points = CreateTestKDCalcPoints(5);
    TEST_ASSERT_NOT_NULL(points);
    result = SelectSplitDimension(points, 5);
    TEST_ASSERT_TRUE(result == 0 || result == 1);
    
    free(points);
    TEST_MESSAGE("KDTree select split dimension test completed");
}

void test_kdtree_build_tree(void) {
    TEST_MESSAGE("Start KDTree build tree test");
    
    KDNode* root = BuildKDTree(NULL, 0, 0);
    TEST_ASSERT_NULL(root);
    
    KDCalcPoint* points = CreateTestKDCalcPoints(1);
    TEST_ASSERT_NOT_NULL(points);
    root = BuildKDTree(points, 1, 0);
    TEST_ASSERT_NOT_NULL(root);
    TEST_ASSERT_EQUAL_FLOAT(5.0, root->latitude);
    TEST_ASSERT_EQUAL_FLOAT(2.0, root->longitude);
    TEST_ASSERT_EQUAL_INT64(1, root->id);
    TEST_ASSERT_NULL(root->left);
    TEST_ASSERT_NULL(root->right);
    DestroyKDNode(root);
    
    free(points);
    points = CreateTestKDCalcPoints(7);
    TEST_ASSERT_NOT_NULL(points);
    root = BuildKDTree(points, 7, 0);
    TEST_ASSERT_NOT_NULL(root);
    TEST_ASSERT_TRUE(ValidateKDNode(root, 0));
    DestroyKDNode(root);
    
    free(points);
    TEST_MESSAGE("KDTree build tree test completed");
}

void test_kdtree_insert_node(void) {
    TEST_MESSAGE("Start KDTree insert node test");
    
    KDNode* root = InsertKDNode(NULL, 10.0, 20.0, 1, 0);
    TEST_ASSERT_NOT_NULL(root);
    TEST_ASSERT_EQUAL_FLOAT(10.0, root->latitude);
    TEST_ASSERT_EQUAL_FLOAT(20.0, root->longitude);
    TEST_ASSERT_EQUAL_INT64(1, root->id);
    TEST_ASSERT_EQUAL_INT(0, root->split_dim);
    TEST_ASSERT_NULL(root->left);
    TEST_ASSERT_NULL(root->right);
    
    root = InsertKDNode(root, 5.0, 15.0, 2, 1);
    TEST_ASSERT_NOT_NULL(root->left);
    TEST_ASSERT_EQUAL_FLOAT(5.0, root->left->latitude);
    TEST_ASSERT_EQUAL_FLOAT(15.0, root->left->longitude);
    TEST_ASSERT_EQUAL_INT64(2, root->left->id);
    
    root = InsertKDNode(root, 15.0, 25.0, 3, 1);
    TEST_ASSERT_NOT_NULL(root->right);
    TEST_ASSERT_EQUAL_FLOAT(15.0, root->right->latitude);
    TEST_ASSERT_EQUAL_FLOAT(25.0, root->right->longitude);
    TEST_ASSERT_EQUAL_INT64(3, root->right->id);
    
    TEST_ASSERT_TRUE(ValidateKDNode(root, 0));
    
    DestroyKDNode(root);
    TEST_MESSAGE("KDTree insert node test completed");
}

void test_kdtree_insert_tree(void) {
    TEST_MESSAGE("Start KDTree insert tree test");
    
    KDTree* tree = (KDTree*)malloc(sizeof(KDTree));
    TEST_ASSERT_NOT_NULL(tree);
    tree->root = NULL;
    tree->size = 0;
    tree->heightIndex = 0;
    
    bool result = InsertKDTree(tree, 10.0, 20.0, 1);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(1, tree->size);
    TEST_ASSERT_NOT_NULL(tree->root);
    TEST_ASSERT_EQUAL_FLOAT(10.0, tree->root->latitude);
    TEST_ASSERT_EQUAL_FLOAT(20.0, tree->root->longitude);
    
    result = InsertKDTree(tree, 5.0, 15.0, 2);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(2, tree->size);
    
    result = InsertKDTree(tree, 15.0, 25.0, 3);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(3, tree->size);
    
    TEST_ASSERT_TRUE(ValidateKDNode(tree->root, 0));
    
    result = InsertKDTree(NULL, 1.0, 2.0, 4);
    TEST_ASSERT_FALSE(result);
    
    DestroyKDTree(tree);
    TEST_MESSAGE("KDTree insert tree test completed");
}

void test_kdtree_search_within_distance(void) {
    TEST_MESSAGE("Start KDTree search within distance test");
    
    KDCalcPoint* points = CreateTestKDCalcPoints(5);
    TEST_ASSERT_NOT_NULL(points);
    KDNode* root = BuildKDTree(points, 5, 0);
    TEST_ASSERT_NOT_NULL(root);
    
    double distSqr = KDTreeSearchNodeWithinDistance(root, root->latitude, root->longitude, 1.0);
    TEST_ASSERT_TRUE(distSqr < 1.0);
    
    distSqr = KDTreeSearchNodeWithinDistance(root, 1000.0, 1000.0, 1.0);
    TEST_ASSERT_EQUAL_DOUBLE(INFINITY, distSqr);
    
    distSqr = KDTreeSearchNodeWithinDistance(NULL, 0.0, 0.0, 1.0);
    TEST_ASSERT_EQUAL_DOUBLE(INFINITY, distSqr);
    
    DestroyKDNode(root);
    free(points);
    TEST_MESSAGE("KDTree search within distance test completed");
}

void test_kdtree_exist_within_distance(void) {
    TEST_MESSAGE("Start KDTree exist within distance test");
    
    KDTree* tree = (KDTree*)malloc(sizeof(KDTree));
    TEST_ASSERT_NOT_NULL(tree);
    tree->size = 0;
    tree->heightIndex = 0;
    
    KDCalcPoint* points = CreateTestKDCalcPoints(5);
    TEST_ASSERT_NOT_NULL(points);
    tree->root = BuildKDTree(points, 5, 0);
    TEST_ASSERT_NOT_NULL(tree->root);
    
    bool exists = KDTreeExistWithinDistance(tree, 5.0, 2.0, 1.0);
    TEST_ASSERT_TRUE(exists);
    
    exists = KDTreeExistWithinDistance(tree, 1000.0, 1000.0, 1.0);
    TEST_ASSERT_FALSE(exists);
    
    exists = KDTreeExistWithinDistance(NULL, 0.0, 0.0, 1.0);
    TEST_ASSERT_FALSE(exists);
    
    KDTree emptyTree = {NULL, 0, 0};
    exists = KDTreeExistWithinDistance(&emptyTree, 0.0, 0.0, 1.0);
    TEST_ASSERT_FALSE(exists);
    
    DestroyKDTree(tree);
    free(points);
    TEST_MESSAGE("KDTree exist within distance test completed");
}

void test_kdtree_destroy_node(void) {
    TEST_MESSAGE("Start KDTree destroy node test");
    KDNode* root = InsertKDNode(NULL, 10.0, 20.0, 1, 0);
    root = InsertKDNode(root, 5.0, 15.0, 2, 1);
    root = InsertKDNode(root, 15.0, 25.0, 3, 1);
    
    TEST_ASSERT_NOT_NULL(root);
    TEST_ASSERT_NOT_NULL(root->left);
    TEST_ASSERT_NOT_NULL(root->right);
    DestroyKDNode(root);
    DestroyKDNode(NULL);
    
    TEST_MESSAGE("KDTree destroy node test completed");
}

void test_kdtree_destroy_tree(void) {
    TEST_MESSAGE("Start KDTree destroy tree test");
    KDTree* tree = (KDTree*)malloc(sizeof(KDTree));
    TEST_ASSERT_NOT_NULL(tree);
    tree->root = NULL;
    tree->size = 0;
    tree->heightIndex = 0;
    InsertKDTree(tree, 10.0, 20.0, 1);
    InsertKDTree(tree, 5.0, 15.0, 2);
    InsertKDTree(tree, 15.0, 25.0, 3);
    
    TEST_ASSERT_NOT_NULL(tree->root);
    TEST_ASSERT_EQUAL_INT(3, tree->size);
    DestroyKDTree(tree);
    DestroyKDTree(NULL);
    
    TEST_MESSAGE("KDTree destroy tree test completed");
}

void test_kdtree_comprehensive(void) {
    TEST_MESSAGE("Start KDTree comprehensive test");
    const int point_count = 100;
    KDCalcPoint* points = (KDCalcPoint*)malloc(point_count * sizeof(KDCalcPoint));
    TEST_ASSERT_NOT_NULL(points);
    for (int i = 0; i < point_count; i++) {
        points[i].latitude = (float)(rand() % 1000) / 10.0;  // 0-99.9
        points[i].longitude = (float)(rand() % 1000) / 10.0; // 0-99.9
        points[i].id = i + 1;
        points[i].lat_sum = points[i].latitude * (i + 1);
        points[i].lon_sum = points[i].longitude * (i + 1);
        points[i].lat_square_sum = points[i].latitude * points[i].latitude * (i + 1);
        points[i].lon_square_sum = points[i].longitude * points[i].longitude * (i + 1);
    }
    
    KDNode* root = BuildKDTree(points, point_count, 0);
    TEST_ASSERT_NOT_NULL(root);
    TEST_ASSERT_TRUE(ValidateKDNode(root, 0));
    
    KDTree* tree = (KDTree*)malloc(sizeof(KDTree));
    TEST_ASSERT_NOT_NULL(tree);
    tree->root = root;
    tree->size = point_count;
    tree->heightIndex = 0;
    
    for (int i = 0; i < 10; i++) {
        float queryLat = (float)(rand() % 1000) / 10.0;
        float queryLon = (float)(rand() % 1000) / 10.0;
        float distance = 5.0;
        
        bool exists = KDTreeExistWithinDistance(tree, queryLat, queryLon, distance);
        double distSqr = KDTreeSearchNodeWithinDistance(tree->root, queryLat, queryLon, distance);
        
        if (exists)
            TEST_ASSERT_TRUE(distSqr < distance * distance);
        else
            TEST_ASSERT_EQUAL_DOUBLE(INFINITY, distSqr);
    }
    
    DestroyKDTree(tree);
    free(points);
    
    TEST_MESSAGE("KDTree comprehensive test completed");
}

void test_kdtree2d(void) {
    TEST_MESSAGE("=== Starting KDTree 2D Tests ===");    
    RUN_TEST(test_kdtree_select_split_dimension);
    RUN_TEST(test_kdtree_build_tree);
    RUN_TEST(test_kdtree_insert_node);
    RUN_TEST(test_kdtree_insert_tree);
    RUN_TEST(test_kdtree_search_within_distance);
    RUN_TEST(test_kdtree_exist_within_distance);
    RUN_TEST(test_kdtree_destroy_node);
    RUN_TEST(test_kdtree_destroy_tree);
    RUN_TEST(test_kdtree_comprehensive);
    TEST_MESSAGE("=== KDTree 2D Tests Completed ===");
}
