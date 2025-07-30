#include "index.h"
#include <string.h>
#include <math.h>
#include <float.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define INITIAL_CAPACITY 4

// ============= IndexArray 实现 =============

IndexArray* index_array_create(void) {
    IndexArray *arr = malloc(sizeof(IndexArray));
    if (!arr) return NULL;
    
    arr->indices = malloc(INITIAL_CAPACITY * sizeof(int));
    if (!arr->indices) {
        free(arr);
        return NULL;
    }
    
    arr->size = 0;
    arr->capacity = INITIAL_CAPACITY;
    return arr;
}

void index_array_destroy(IndexArray *arr) {
    if (arr) {
        free(arr->indices);
        free(arr);
    }
}

bool index_array_add(IndexArray *arr, int index) {
    if (!arr) return false;
    
    // 检查容量是否足够
    if (arr->size >= arr->capacity) {
        int new_capacity = arr->capacity * 2;
        int *new_indices = realloc(arr->indices, new_capacity * sizeof(int));
        if (!new_indices) return false;
        
        arr->indices = new_indices;
        arr->capacity = new_capacity;
    }
    
    arr->indices[arr->size++] = index;
    return true;
}

void index_array_print(const IndexArray *arr) {
    if (!arr) return;
    
    printf("[");
    for (int i = 0; i < arr->size; i++) {
        printf("%d", arr->indices[i]);
        if (i < arr->size - 1) printf(", ");
    }
    printf("]");
}

// ============= AVL树辅助函数 =============

static int get_height(AVLNode *node) {
    return node ? node->height : 0;
}

static int get_balance_factor(AVLNode *node) {
    return node ? get_height(node->left) - get_height(node->right) : 0;
}

static void update_height(AVLNode *node) {
    if (node) {
        node->height = 1 + MAX(get_height(node->left), get_height(node->right));
    }
}

static AVLNode* create_node(double value, int array_index) {
    AVLNode *node = malloc(sizeof(AVLNode));
    if (!node) return NULL;
    
    node->value = value;
    node->indices = index_array_create();
    if (!node->indices) {
        free(node);
        return NULL;
    }
    
    if (!index_array_add(node->indices, array_index)) {
        index_array_destroy(node->indices);
        free(node);
        return NULL;
    }
    
    node->height = 1;
    node->left = NULL;
    node->right = NULL;
    return node;
}

static void destroy_node(AVLNode *node) {
    if (node) {
        index_array_destroy(node->indices);
        free(node);
    }
}

// ============= AVL树旋转操作 =============

static AVLNode* rotate_right(AVLNode *y) {
    AVLNode *x = y->left;
    AVLNode *T2 = x->right;
    
    // 执行旋转
    x->right = y;
    y->left = T2;
    
    // 更新高度
    update_height(y);
    update_height(x);
    
    return x;
}

static AVLNode* rotate_left(AVLNode *x) {
    AVLNode *y = x->right;
    AVLNode *T2 = y->left;
    
    // 执行旋转
    y->left = x;
    x->right = T2;
    
    // 更新高度
    update_height(x);
    update_height(y);
    
    return y;
}

// ============= AVL树基本操作 =============

AVLTree* avl_tree_create(void) {
    AVLTree *tree = malloc(sizeof(AVLTree));
    if (!tree) return NULL;
    
    tree->root = NULL;
    tree->node_count = 0;
    return tree;
}

static void destroy_nodes(AVLNode *node) {
    if (node) {
        destroy_nodes(node->left);
        destroy_nodes(node->right);
        destroy_node(node);
    }
}

void avl_tree_destroy(AVLTree *tree) {
    if (tree) {
        destroy_nodes(tree->root);
        free(tree);
    }
}

static AVLNode* insert_node(AVLNode *node, double value, int array_index, bool *success) {
    // 1. 标准BST插入
    if (!node) {
        *success = true;
        return create_node(value, array_index);
    }
    
    if (fabs(value - node->value) < 1e-9) {
        // 值相等，添加到现有节点的索引数组中
        *success = index_array_add(node->indices, array_index);
        return node;
    } else if (value < node->value) {
        node->left = insert_node(node->left, value, array_index, success);
    } else {
        node->right = insert_node(node->right, value, array_index, success);
    }
    
    if (!*success) return node;
    
    // 2. 更新节点高度
    update_height(node);
    
    // 3. 获取平衡因子
    int balance = get_balance_factor(node);
    
    // 4. 如果节点不平衡，执行旋转
    
    // Left Left Case
    if (balance > 1 && value < node->left->value) {
        return rotate_right(node);
    }
    
    // Right Right Case
    if (balance < -1 && value > node->right->value) {
        return rotate_left(node);
    }
    
    // Left Right Case
    if (balance > 1 && value > node->left->value) {
        node->left = rotate_left(node->left);
        return rotate_right(node);
    }
    
    // Right Left Case
    if (balance < -1 && value < node->right->value) {
        node->right = rotate_right(node->right);
        return rotate_left(node);
    }
    
    return node;
}

bool avl_tree_insert(AVLTree *tree, double value, int array_index) {
    if (!tree) return false;
    
    bool success = false;
    int old_count = tree->node_count;
    tree->root = insert_node(tree->root, value, array_index, &success);
    
    if (success && tree->node_count == old_count) {
        // 新节点被创建
        tree->node_count++;
    }
    
    return success;
}

static AVLNode* find_min(AVLNode *node) {
    while (node && node->left) {
        node = node->left;
    }
    return node;
}

static AVLNode* delete_node(AVLNode *root, double value, bool *found) {
    if (!root) {
        *found = false;
        return root;
    }
    
    if (fabs(value - root->value) < 1e-9) {
        *found = true;
        
        if (!root->left || !root->right) {
            AVLNode *temp = root->left ? root->left : root->right;
            
            if (!temp) {
                // 没有子节点
                destroy_node(root);
                return NULL;
            } else {
                // 有一个子节点
                IndexArray *old_indices = root->indices;
                root->indices = temp->indices;
                temp->indices = old_indices;  // 交换indices
                *root = *temp;  // 复制其他字段
                destroy_node(temp);  // 安全释放，因为temp现在持有原来的indices
            }
        } else {
            // 有两个子节点
            AVLNode *temp = find_min(root->right);
            
            // 只交换必要的数据，而不是整个indices结构
            double temp_value = temp->value;
            IndexArray *temp_indices = temp->indices;
            temp->value = root->value;
            temp->indices = root->indices;
            root->value = temp_value;
            root->indices = temp_indices;
            
            // 现在递归删除原始节点（数据已经被交换）
            bool temp_found = false;
            root->right = delete_node(root->right, temp->value, &temp_found);
        }
    } else if (value < root->value) {
        root->left = delete_node(root->left, value, found);
    } else {
        root->right = delete_node(root->right, value, found);
    }
    
    if (!root) return root;
    
    // 更新高度和平衡操作保持不变
    update_height(root);
    int balance = get_balance_factor(root);
    
    // 执行旋转
    if (balance > 1 && get_balance_factor(root->left) >= 0) {
        return rotate_right(root);
    }
    
    if (balance > 1 && get_balance_factor(root->left) < 0) {
        root->left = rotate_left(root->left);
        return rotate_right(root);
    }
    
    if (balance < -1 && get_balance_factor(root->right) <= 0) {
        return rotate_left(root);
    }
    
    if (balance < -1 && get_balance_factor(root->right) > 0) {
        root->right = rotate_right(root->right);
        return rotate_left(root);
    }
    
    return root;
}

bool avl_tree_delete_value(AVLTree *tree, double value) {
    if (!tree) return false;
    
    bool found = false;
    tree->root = delete_node(tree->root, value, &found);
    
    if (found) {
        tree->node_count--;
    }
    
    return found;
}

static AVLNode* search_node(AVLNode *node, double value) {
    if (!node || fabs(value - node->value) < 1e-9) {
        return node;
    }
    
    if (value < node->value) {
        return search_node(node->left, value);
    } else {
        return search_node(node->right, value);
    }
}

AVLNode* avl_tree_search(AVLTree *tree, double value) {
    if (!tree) return NULL;
    return search_node(tree->root, value);
}

// ============= 范围查询实现 =============

static void range_query_recursive(AVLNode *node, double min_value, double max_value, 
                                 int **result, int *count, int *capacity) {
    if (!node) return;
    
    // 如果当前节点的值在范围内
    if (node->value >= min_value && node->value <= max_value) {
        // 添加所有索引到结果中
        for (int i = 0; i < node->indices->size; i++) {
            // 检查容量
            if (*count >= *capacity) {
                *capacity *= 2;
                *result = realloc(*result, *capacity * sizeof(int));
            }
            (*result)[(*count)++] = node->indices->indices[i];
        }
    }
    
    // 递归搜索左子树（如果需要）
    if (node->value > min_value) {
        range_query_recursive(node->left, min_value, max_value, result, count, capacity);
    }
    
    // 递归搜索右子树（如果需要）
    if (node->value < max_value) {
        range_query_recursive(node->right, min_value, max_value, result, count, capacity);
    }
}

QueryResult* avl_tree_range_query(AVLTree *tree, double min_value, double max_value) {
    if (!tree) return NULL;
    
    QueryResult *result = malloc(sizeof(QueryResult));
    if (!result) return NULL;
    
    int capacity = INITIAL_CAPACITY;
    result->indices = malloc(capacity * sizeof(int));
    if (!result->indices) {
        free(result);
        return NULL;
    }
    
    result->count = 0;
    
    range_query_recursive(tree->root, min_value, max_value, 
                         &result->indices, &result->count, &capacity);
    
    return result;
}

void query_result_destroy(QueryResult *result) {
    if (result) {
        free(result->indices);
        free(result);
    }
}

// ============= 辅助函数 =============

static void print_inorder_recursive(AVLNode *node) {
    if (node) {
        print_inorder_recursive(node->left);
        printf("Value: %.2f, Indices: ", node->value);
        index_array_print(node->indices);
        printf("\n");
        print_inorder_recursive(node->right);
    }
}

void avl_tree_print_inorder(AVLTree *tree) {
    if (tree) {
        print_inorder_recursive(tree->root);
    }
}

int avl_tree_get_height(AVLTree *tree) {
    return tree ? get_height(tree->root) : 0;
}

bool avl_tree_is_empty(AVLTree *tree) {
    return !tree || !tree->root;
}

AVLTree* avl_tree_from_array(const double *values, int array_size) {
    if (!values || array_size <= 0) return NULL;
    
    AVLTree *tree = avl_tree_create();
    if (!tree) return NULL;
    
    for (int i = 0; i < array_size; i++) {
        if (!avl_tree_insert(tree, values[i], i)) {
            avl_tree_destroy(tree);
            return NULL;
        }
    }
    
    return tree;
}

bool avl_tree_delete_index(AVLTree *tree, double value, int array_index) {
    if (!tree) return false;
    
    AVLNode *node = avl_tree_search(tree, value);
    if (!node) return false;
    
    // 查找并删除特定的索引
    for (int i = 0; i < node->indices->size; i++) {
        if (node->indices->indices[i] == array_index) {
            // 移动后面的元素前移
            for (int j = i; j < node->indices->size - 1; j++) {
                node->indices->indices[j] = node->indices->indices[j + 1];
            }
            node->indices->size--;
            
            // 如果索引数组为空，删除整个节点
            if (node->indices->size == 0) {
                return avl_tree_delete_value(tree, value);
            }
            
            return true;
        }
    }
    
    return false;
}

// ============= 测试辅助函数实现 =============

int avl_node_get_height(AVLNode *node) {
    return get_height(node);
}

int avl_node_get_balance_factor(AVLNode *node) {
    return get_balance_factor(node);
}

static bool validate_balance_recursive(AVLNode *node) {
    if (!node) return true;
    
    int balance = get_balance_factor(node);
    if (abs(balance) > 1) return false;
    
    return validate_balance_recursive(node->left) && validate_balance_recursive(node->right);
}

bool avl_tree_validate_balance(AVLTree *tree) {
    if (!tree) return false;
    return validate_balance_recursive(tree->root);
}

static bool validate_bst_recursive(AVLNode *node, double min_val, double max_val) {
    if (!node) return true;
    
    if (node->value <= min_val || node->value >= max_val) {
        return false;
    }
    
    return validate_bst_recursive(node->left, min_val, node->value) &&
           validate_bst_recursive(node->right, node->value, max_val);
}

bool avl_tree_validate_bst(AVLTree *tree) {
    if (!tree) return false;
    return validate_bst_recursive(tree->root, -DBL_MAX, DBL_MAX);
}