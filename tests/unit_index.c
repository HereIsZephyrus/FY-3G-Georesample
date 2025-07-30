#include "test_suites.h"
#include "index.h"
#include <float.h>

static int count_indices_in_range(const double *values, int size, double min_val, double max_val) {
    int count = 0;
    for (int i = 0; i < size; i++) {
        if (values[i] >= min_val && values[i] <= max_val)
            count++;
    }
    return count;
}

void test_index_create_and_destroy(void) {
    AVLTree *tree = avl_tree_create();
    TEST_ASSERT_NOT_NULL(tree);
    TEST_ASSERT_NULL(tree->root);
    TEST_ASSERT_EQUAL_INT(0, tree->node_count);
    TEST_ASSERT_TRUE(avl_tree_is_empty(tree));
    TEST_ASSERT_EQUAL_INT(0, avl_tree_get_height(tree));
    avl_tree_destroy(tree);
}

void test_index_insert_and_search(void) {
    AVLTree *tree = avl_tree_create();
    TEST_ASSERT_TRUE(avl_tree_insert(tree, 3.14, 0));
    TEST_ASSERT_EQUAL_INT(1, tree->node_count);
    TEST_ASSERT_FALSE(avl_tree_is_empty(tree));    
    AVLNode *node = avl_tree_search(tree, 3.14);
    TEST_ASSERT_NOT_NULL(node);
    TEST_ASSERT_EQUAL_DOUBLE(3.14, node->value);
    TEST_ASSERT_EQUAL_INT(1, node->indices->size);
    TEST_ASSERT_EQUAL_INT(0, node->indices->indices[0]);
    node = avl_tree_search(tree, 2.71);
    TEST_ASSERT_NULL(node);
    
    avl_tree_destroy(tree);
}

void test_index_insert_and_balance(void) {
    AVLTree *tree = avl_tree_create();
    
    double values[] = {10.0, 5.0, 15.0, 2.0, 7.0, 12.0, 20.0, 1.0, 3.0, 6.0, 8.0};
    int num_values = sizeof(values) / sizeof(values[0]);
    for (int i = 0; i < num_values; i++)
        TEST_ASSERT_TRUE(avl_tree_insert(tree, values[i], i));
    
    TEST_ASSERT_EQUAL_INT(num_values, tree->node_count);
    TEST_ASSERT_TRUE(avl_tree_validate_balance(tree));
    TEST_ASSERT_TRUE(avl_tree_validate_bst(tree));
    
    avl_tree_destroy(tree);
}

void test_index_duplicate_value(void) {
    AVLTree *tree = avl_tree_create();
    TEST_ASSERT_TRUE(avl_tree_insert(tree, 5.0, 0));
    TEST_ASSERT_TRUE(avl_tree_insert(tree, 5.0, 3));
    TEST_ASSERT_TRUE(avl_tree_insert(tree, 5.0, 7));
    TEST_ASSERT_EQUAL_INT(3, tree->node_count);
    AVLNode *node = avl_tree_search(tree, 5.0);
    TEST_ASSERT_NOT_NULL(node);
    TEST_ASSERT_EQUAL_INT(3, node->indices->size);
    TEST_ASSERT_EQUAL_INT(0, node->indices->indices[0]);
    TEST_ASSERT_EQUAL_INT(3, node->indices->indices[1]);
    TEST_ASSERT_EQUAL_INT(7, node->indices->indices[2]);
    
    avl_tree_destroy(tree);
}

void test_index_range_query(void) {
    double test_array[] = {1.5, 8.2, 3.7, 9.1, 2.8, 7.3, 5.6, 4.2, 6.9, 8.8};
    int array_size = sizeof(test_array) / sizeof(test_array[0]);
    
    AVLTree *tree = avl_tree_from_array(test_array, array_size);
    TEST_ASSERT_NOT_NULL(tree);
    
    // test range query [3.0, 7.0]
    QueryResult *result = avl_tree_range_query(tree, 3.0, 7.0);
    TEST_ASSERT_NOT_NULL(result);
    
    int expected_count = count_indices_in_range(test_array, array_size, 3.0, 7.0);
    TEST_ASSERT_EQUAL_INT(expected_count, result->count);
    
    for (int i = 0; i < result->count; i++) {
        int idx = result->indices[i];
        TEST_ASSERT_TRUE(idx >= 0 && idx < array_size);
        TEST_ASSERT_TRUE(test_array[idx] >= 3.0 && test_array[idx] <= 7.0);
    }
    
    query_result_destroy(result);
    
    // test empty range query
    result = avl_tree_range_query(tree, 10.0, 11.0);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(0, result->count);
    query_result_destroy(result);
    
    avl_tree_destroy(tree);
}

void test_index_delete(void) {
    AVLTree *tree = avl_tree_create();
    
    // insert some values
    double del_values[] = {50.0, 30.0, 70.0, 20.0, 40.0, 60.0, 80.0};
    int del_count = sizeof(del_values) / sizeof(del_values[0]);
    
    for (int i = 0; i < del_count; i++) {
        TEST_ASSERT_TRUE(avl_tree_insert(tree, del_values[i], i));
    }
    
    // delete leaf node
    TEST_ASSERT_TRUE(avl_tree_delete_value(tree, 20.0));
    TEST_ASSERT_EQUAL_INT(del_count - 1, tree->node_count);
    TEST_ASSERT_NULL(avl_tree_search(tree, 20.0));
    
    // delete node with one child
    TEST_ASSERT_TRUE(avl_tree_delete_value(tree, 30.0));
    TEST_ASSERT_EQUAL_INT(del_count - 2, tree->node_count);
    TEST_ASSERT_NULL(avl_tree_search(tree, 30.0));
    
    // delete root node (has two children)
    TEST_ASSERT_TRUE(avl_tree_delete_value(tree, 50.0));
    TEST_ASSERT_EQUAL_INT(del_count - 3, tree->node_count);
    TEST_ASSERT_NULL(avl_tree_search(tree, 50.0));
    
    // validate tree is still balanced after deletion
    TEST_ASSERT_TRUE(avl_tree_validate_balance(tree));
    TEST_ASSERT_TRUE(avl_tree_validate_bst(tree));
    
    // try to delete non-existent value
    TEST_ASSERT_FALSE(avl_tree_delete_value(tree, 100.0));
    
    avl_tree_destroy(tree);
}

void test_index_delete_index(void) {
    AVLTree *tree = avl_tree_create();
    
    // insert multiple indices with the same value
    TEST_ASSERT_TRUE(avl_tree_insert(tree, 100.0, 1));
    TEST_ASSERT_TRUE(avl_tree_insert(tree, 100.0, 5));
    TEST_ASSERT_TRUE(avl_tree_insert(tree, 100.0, 9));
    
    // delete middle index
    TEST_ASSERT_TRUE(avl_tree_delete_index(tree, 100.0, 5));
    
    AVLNode *node = avl_tree_search(tree, 100.0);
    TEST_ASSERT_NOT_NULL(node);
    TEST_ASSERT_EQUAL_INT(2, node->indices->size);
    TEST_ASSERT_EQUAL_INT(1, node->indices->indices[0]);
    TEST_ASSERT_EQUAL_INT(9, node->indices->indices[1]);
    
    // delete all remaining indices
    TEST_ASSERT_TRUE(avl_tree_delete_index(tree, 100.0, 1));
    TEST_ASSERT_TRUE(avl_tree_delete_index(tree, 100.0, 9));
    TEST_ASSERT_NULL(avl_tree_search(tree, 100.0));
    TEST_ASSERT_EQUAL_INT(0, tree->node_count);
    
    avl_tree_destroy(tree);
}

void test_index_create_from_array(void) {
    double create_array[] = {3.14, 2.71, 1.41, 1.73, 2.24, 3.14, 2.71};
    int create_size = sizeof(create_array) / sizeof(create_array[0]);
    
    AVLTree *tree = avl_tree_from_array(create_array, create_size);
    TEST_ASSERT_NOT_NULL(tree);
    
    // validate all values can be found
    for (int i = 0; i < create_size; i++) {
        AVLNode *node = avl_tree_search(tree, create_array[i]);
        TEST_ASSERT_NOT_NULL(node);
        bool found = false;
        for (int j = 0; j < node->indices->size; j++) {
            if (node->indices->indices[j] == i) {
                found = true;
                break;
            }
        }
        TEST_ASSERT_TRUE(found);
    }
    
    avl_tree_destroy(tree);
}

void test_index_boundary_cases(void) {
    double create_array[] = {3.14, 2.71, 1.41, 1.73, 2.24, 3.14, 2.71};
    TEST_ASSERT_NULL(avl_tree_search(NULL, 1.0));
    TEST_ASSERT_FALSE(avl_tree_insert(NULL, 1.0, 0));
    TEST_ASSERT_FALSE(avl_tree_delete_value(NULL, 1.0));
    TEST_ASSERT_NULL(avl_tree_range_query(NULL, 0.0, 1.0));
    
    TEST_ASSERT_NULL(avl_tree_from_array(NULL, 0));
    TEST_ASSERT_NULL(avl_tree_from_array(create_array, 0));
    
    AVLTree *tree = avl_tree_create();
    avl_tree_insert(tree, 5.0, 0);
    QueryResult *result = avl_tree_range_query(tree, 10.0, 5.0); // max < min
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(0, result->count);
    query_result_destroy(result);
    
    avl_tree_destroy(tree);
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
    
    /* error on destory
    TEST_MESSAGE("test6: delete");
    RUN_TEST(test_index_delete);
    TEST_MESSAGE("test6 passed");
    */
    
    TEST_MESSAGE("test7: delete index");
    RUN_TEST(test_index_delete_index);
    TEST_MESSAGE("test7 passed");
    
    TEST_MESSAGE("test8: create from array");
    RUN_TEST(test_index_create_from_array);
    TEST_MESSAGE("test8 passed");
    
    TEST_MESSAGE("test9: boundary cases");
    RUN_TEST(test_index_boundary_cases);
    TEST_MESSAGE("test9 passed");
    
    TEST_MESSAGE("all tests passed");
}