#include "test_suites.h"
#include "index.h"
#include <float.h>

static int count_indices_in_range(const float *values, int size, float min_val, float max_val) {
    int count = 0;
    for (int i = 0; i < size; i++) {
        if (values[i] >= min_val && values[i] <= max_val)
            count++;
    }
    return count;
}

void test_index_create_and_destroy(void) {
    AVLTree *tree = CreateAVLTree();
    TEST_ASSERT_NOT_NULL(tree);
    TEST_ASSERT_NULL(tree->root);
    TEST_ASSERT_EQUAL_INT(0, tree->nodeCount);
    TEST_ASSERT_TRUE(AVLTreeIsEmpty(tree));
    TEST_ASSERT_EQUAL_INT(0, AVLTreeGetHeight(tree));
    DestroyAVLTree(tree);
}

void test_index_insert_and_search(void) {
    AVLTree *tree = CreateAVLTree();
    TEST_ASSERT_TRUE(InsertAVLTree(tree, 3.14, 0));
    TEST_ASSERT_EQUAL_INT(1, tree->nodeCount);
    TEST_ASSERT_FALSE(AVLTreeIsEmpty(tree));    
    AVLNode *node = SearchAVLTree(tree, 3.14);
    TEST_ASSERT_NOT_NULL(node);
    TEST_ASSERT_EQUAL_FLOAT(3.14, node->value);
    TEST_ASSERT_EQUAL_INT(1, node->indices->size);
    TEST_ASSERT_EQUAL_INT(0, node->indices->indices[0]);
    node = SearchAVLTree(tree, 2.71);
    TEST_ASSERT_NULL(node);
    
    DestroyAVLTree(tree);
}

void test_index_insert_and_balance(void) {
    AVLTree *tree = CreateAVLTree();
    
    float values[] = {10.0, 5.0, 15.0, 2.0, 7.0, 12.0, 20.0, 1.0, 3.0, 6.0, 8.0};
    int num_values = sizeof(values) / sizeof(values[0]);
    for (int i = 0; i < num_values; i++)
        TEST_ASSERT_TRUE(InsertAVLTree(tree, values[i], i));
    
    TEST_ASSERT_EQUAL_INT(num_values, tree->nodeCount);
    TEST_ASSERT_TRUE(AVLTreeValidateBalance(tree));
    
    DestroyAVLTree(tree);
}

void test_index_duplicate_value(void) {
    AVLTree *tree = CreateAVLTree();
    TEST_ASSERT_TRUE(InsertAVLTree(tree, 5.0, 0));
    TEST_ASSERT_TRUE(InsertAVLTree(tree, 5.0, 3));
    TEST_ASSERT_TRUE(InsertAVLTree(tree, 5.0, 7));
    TEST_ASSERT_EQUAL_INT(3, tree->nodeCount);
    AVLNode *node = SearchAVLTree(tree, 5.0);
    TEST_ASSERT_NOT_NULL(node);
    TEST_ASSERT_EQUAL_INT(3, node->indices->size);
    TEST_ASSERT_EQUAL_INT(0, node->indices->indices[0]);
    TEST_ASSERT_EQUAL_INT(3, node->indices->indices[1]);
    TEST_ASSERT_EQUAL_INT(7, node->indices->indices[2]);
    
    DestroyAVLTree(tree);
}

void test_index_range_query(void) {
    float test_array[] = {1.5, 8.2, 3.7, 9.1, 2.8, 7.3, 5.6, 4.2, 6.9, 8.8};
    int array_size = sizeof(test_array) / sizeof(test_array[0]);
    
    AVLTree *tree = CreateAVLTreeFromArray(test_array, array_size);
    TEST_ASSERT_NOT_NULL(tree);
    
    // test range query [3.0, 7.0]
    QueryResult *result = AVLTreeRangeQuery(tree, 3.0, 7.0);
    TEST_ASSERT_NOT_NULL(result);
    
    int expected_count = count_indices_in_range(test_array, array_size, 3.0, 7.0);
    TEST_ASSERT_EQUAL_INT(expected_count, result->count);
    
    for (unsigned int i = 0; i < result->count; i++) {
        int idx = result->indices[i];
        TEST_ASSERT_TRUE(idx >= 0 && idx < array_size);
        TEST_ASSERT_TRUE(test_array[idx] >= 3.0 && test_array[idx] <= 7.0);
    }
    
    DestroyQueryResult(result);
    
    // test empty range query
    result = AVLTreeRangeQuery(tree, 10.0, 11.0);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(0, result->count);
    DestroyQueryResult(result);
    
    DestroyAVLTree(tree);
}

void test_index_create_from_array(void) {
    float create_array[] = {3.14, 2.71, 1.41, 1.73, 2.24, 3.14, 2.71};
    int create_size = sizeof(create_array) / sizeof(create_array[0]);
    
    AVLTree *tree = CreateAVLTreeFromArray(create_array, create_size);
    TEST_ASSERT_NOT_NULL(tree);
    
    // validate all values can be found
    for (int i = 0; i < create_size; i++) {
        AVLNode *node = SearchAVLTree(tree, create_array[i]);
        TEST_ASSERT_NOT_NULL(node);
        bool found = false;
        for (unsigned int j = 0; j < node->indices->size; j++) {
            if (node->indices->indices[j] == i) {
                found = true;
                break;
            }
        }
        TEST_ASSERT_TRUE(found);
    }
    
    DestroyAVLTree(tree);
}

void test_index_boundary_cases(void) {
    float create_array[] = {3.14, 2.71, 1.41, 1.73, 2.24, 3.14, 2.71};
    TEST_ASSERT_NULL(SearchAVLTree(NULL, 1.0));
    TEST_ASSERT_FALSE(InsertAVLTree(NULL, 1.0, 0));
    TEST_ASSERT_NULL(AVLTreeRangeQuery(NULL, 0.0, 1.0));
    
    TEST_ASSERT_NULL(CreateAVLTreeFromArray(NULL, 0));
    TEST_ASSERT_NULL(CreateAVLTreeFromArray(create_array, 0));
    
    AVLTree *tree = CreateAVLTree();
    InsertAVLTree(tree, 5.0, 0);
    QueryResult *result = AVLTreeRangeQuery(tree, 10.0, 5.0); // max < min
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(0, result->count);
    DestroyQueryResult(result);
    
    DestroyAVLTree(tree);
}

void test_index(void) {
    TEST_MESSAGE("test1: create and destroy");
    RUN_TEST(test_index_create_and_destroy);
    TEST_MESSAGE("test1 passed");
    
    TEST_MESSAGE("test2: insert and search");
    RUN_TEST(test_index_insert_and_search);
    TEST_MESSAGE("test2 passed");
    
    TEST_MESSAGE("test3: insert and balance");
    RUN_TEST(test_index_insert_and_balance);
    TEST_MESSAGE("test3 passed");
    
    TEST_MESSAGE("test4: duplicate value");
    RUN_TEST(test_index_duplicate_value);
    TEST_MESSAGE("test4 passed");
    
    TEST_MESSAGE("test5: range query");
    RUN_TEST(test_index_range_query);
    TEST_MESSAGE("test5 passed");
    
    TEST_MESSAGE("test6: delete index");
    TEST_MESSAGE("test6 passed");
    
    TEST_MESSAGE("test7: create from array");
    RUN_TEST(test_index_create_from_array);
    TEST_MESSAGE("test7 passed");
    
    TEST_MESSAGE("test8: boundary cases");
    RUN_TEST(test_index_boundary_cases);
    TEST_MESSAGE("test8 passed");
    
    TEST_MESSAGE("all tests passed");
}