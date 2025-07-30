#ifndef INDEX_H
#define INDEX_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// 动态数组结构，用于存储数组下标
typedef struct {
    int *indices;     // 存储下标的数组
    int size;         // 当前大小
    int capacity;     // 容量
} IndexArray;

// AVL树节点结构
typedef struct AVLNode {
    double value;              // 键值（数组中的值）
    IndexArray *indices;       // 存储具有相同值的所有数组下标
    int height;                // 节点高度
    struct AVLNode *left;      // 左子树
    struct AVLNode *right;     // 右子树
} AVLNode;

// AVL树结构
typedef struct {
    AVLNode *root;             // 根节点
    int node_count;            // 节点数量
} AVLTree;

// 查询结果结构
typedef struct {
    int *indices;              // 查询结果的下标数组
    int count;                 // 结果数量
} QueryResult;

// IndexArray 操作函数
IndexArray* index_array_create(void);
void index_array_destroy(IndexArray *arr);
bool index_array_add(IndexArray *arr, int index);
void index_array_print(const IndexArray *arr);

// AVL树操作函数
AVLTree* avl_tree_create(void);
void avl_tree_destroy(AVLTree *tree);
bool avl_tree_insert(AVLTree *tree, double value, int array_index);
bool avl_tree_delete_value(AVLTree *tree, double value);
bool avl_tree_delete_index(AVLTree *tree, double value, int array_index);
AVLNode* avl_tree_search(AVLTree *tree, double value);

// 范围查询函数
QueryResult* avl_tree_range_query(AVLTree *tree, double min_value, double max_value);
void query_result_destroy(QueryResult *result);

// 辅助函数
void avl_tree_print_inorder(AVLTree *tree);
int avl_tree_get_height(AVLTree *tree);
bool avl_tree_is_empty(AVLTree *tree);

// 便利函数：从数组创建AVL树
AVLTree* avl_tree_from_array(const double *values, int array_size);

// 测试辅助函数（仅用于单元测试）
int avl_node_get_height(AVLNode *node);
int avl_node_get_balance_factor(AVLNode *node);
bool avl_tree_validate_balance(AVLTree *tree);
bool avl_tree_validate_bst(AVLTree *tree);

#endif