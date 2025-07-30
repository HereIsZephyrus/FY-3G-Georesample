#include "index.h"
#include <string.h>
#include <math.h>
#include <float.h>
#include <stdio.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define INITIAL_CAPACITY 4

IndexArray* CreateIndexArray() {
    IndexArray *arr = malloc(sizeof(IndexArray));
    if (!arr) return NULL;
    
    arr->indices = malloc(INITIAL_CAPACITY * sizeof(int));
    if (!arr->indices) {
        fprintf(stderr, "Failed to allocate memory for index array\n");
        free(arr);
        return NULL;
    }
    
    arr->size = 0;
    arr->capacity = INITIAL_CAPACITY;
    return arr;
}

void DestroyIndexArray(IndexArray *arr) {
    if (!arr) return;
    if (arr->indices) 
        free(arr->indices);
    free(arr);
}

bool AppendIndex(IndexArray *arr, int index) {
    if (!arr) return false;
    
    if (arr->size >= arr->capacity) { // dynamically increase the capacity
        unsigned int newCapacity = arr->capacity * 2;
        int *newIndices = realloc(arr->indices, newCapacity * sizeof(int));
        if (!newIndices) return false;
        arr->indices = newIndices;
        arr->capacity = newCapacity;
    }
    arr->indices[arr->size++] = index;
    return true;
}

int GetNodeHeight(AVLNode *node) {
    return node ? node->height : 0;
}

int GetNodeBalanceFactor(AVLNode *node) {
    return node ? GetNodeHeight(node->left) - GetNodeHeight(node->right) : 0;
}

void UpdateNodeHeight(AVLNode *node) {
    if (!node) return;
    node->height = 1 + MAX(GetNodeHeight(node->left), GetNodeHeight(node->right));
}

AVLNode* CreateAVLNode(float value, int arrayIndex) {
    AVLNode *node = malloc(sizeof(AVLNode));
    if (!node) return NULL;
    
    node->value = value;
    node->indices = CreateIndexArray();
    if (!node->indices) {
        fprintf(stderr, "Failed to create index array for AVL node\n");
        free(node);
        return NULL;
    }
    
    if (!AppendIndex(node->indices, arrayIndex)) {
        DestroyIndexArray(node->indices);
        fprintf(stderr, "Failed to append index to AVL node\n");
        free(node);
        return NULL;
    }
    
    node->height = 1;
    node->left = NULL;
    node->right = NULL;
    return node;
}

void DestroyAVLNode(AVLNode *node) {
    if (!node) return;
    DestroyIndexArray(node->indices);
    free(node);
}

AVLNode* RotateRight(AVLNode *y) {
    AVLNode *x = y->left;
    AVLNode *T2 = x->right;
    x->right = y;
    y->left = T2;
    UpdateNodeHeight(y);
    UpdateNodeHeight(x);
    return x;
}

AVLNode* RotateLeft(AVLNode *x) {
    AVLNode *y = x->right;
    AVLNode *T2 = y->left;
    y->left = x;
    x->right = T2;
    UpdateNodeHeight(x);
    UpdateNodeHeight(y);
    return y;
}

AVLTree* CreateAVLTree(void) {
    AVLTree *tree = malloc(sizeof(AVLTree));
    if (!tree) return NULL;
    tree->root = NULL;
    tree->nodeCount = 0;
    return tree;
}

void DestroyAVLNodes(AVLNode *node) {
    if (!node) return;
    DestroyAVLNodes(node->left);
    DestroyAVLNodes(node->right);
    DestroyAVLNode(node);
}

void DestroyAVLTree(AVLTree *tree) {
    if (!tree) return;
    DestroyAVLNodes(tree->root);
    free(tree);
}

AVLNode* InsertAVLNode(AVLNode *node, float value, int arrayIndex, bool *success) {
    if (!node) {
        *success = true;
        return CreateAVLNode(value, arrayIndex);
    }
    
    if (fabs(value - node->value) < 1e-9) { // already exists the value
        *success = AppendIndex(node->indices, arrayIndex);
        return node;
    }
    if (value < node->value)
        node->left = InsertAVLNode(node->left, value, arrayIndex, success);
    else
        node->right = InsertAVLNode(node->right, value, arrayIndex, success);
    
    if (!*success){
        fprintf(stderr, "Failed to insert AVL node\n");
        return node;
    }
    
    UpdateNodeHeight(node);
    
    int balance = GetNodeBalanceFactor(node);
    if (balance > 1 && value < node->left->value) // Left Left Case
        return RotateRight(node);
    if (balance < -1 && value > node->right->value) // Right Right Case
        return RotateLeft(node);
    if (balance > 1 && value > node->left->value){ // Left Right Case
        node->left = RotateLeft(node->left);
        return RotateRight(node);
    }
    if (balance < -1 && value < node->right->value){ // Right Left Case
        node->right = RotateRight(node->right);
        return RotateLeft(node);
    }
    return node;
}

bool InsertAVLTree(AVLTree *tree, float value, int arrayIndex) {
    if (!tree) return false;
    
    bool success = false;
    tree->root = InsertAVLNode(tree->root, value, arrayIndex, &success);
    if (success)
        tree->nodeCount++;
    
    return success;
}

AVLNode* SearchAVLNode(AVLNode *node, float value) {
    if (!node || fabs(value - node->value) < 1e-6) 
        return node;
    
    if (value < node->value) 
        return SearchAVLNode(node->left, value);
    else 
        return SearchAVLNode(node->right, value);
}

AVLNode* SearchAVLTree(AVLTree *tree, float value) {
    if (!tree) return NULL;
    return SearchAVLNode(tree->root, value);
}

void AVLNodeRangeQuery(AVLNode *node, float latMin, float latMax, const float *longitudeArray, float lonMin, float lonMax, int **result, unsigned int *count, unsigned int *capacity) {
    if (!node) return;
    if (node->value >= latMin && node->value <= latMax) {
        for (unsigned int i = 0; i < node->indices->size; i++) {
            // check if the longitude value is in the range
            if (longitudeArray[node->indices->indices[i]] < lonMin || longitudeArray[node->indices->indices[i]] > lonMax)
                continue;
            if (*count >= *capacity) {
                *capacity *= 2;
                *result = realloc(*result, *capacity * sizeof(int));
            }
            (*result)[(*count)++] = node->indices->indices[i];
        }
    }
    
    if (node->value > latMin)
        AVLNodeRangeQuery(node->left, latMin, latMax, longitudeArray, lonMin, lonMax, result, count, capacity);
    
    if (node->value < latMax)
        AVLNodeRangeQuery(node->right, latMin, latMax, longitudeArray, lonMin, lonMax, result, count, capacity);
}

QueryResult* AVLTreeRangeQuery(AVLTree *tree, float latMin, float latMax, const float *longitudeArray, float lonMin, float lonMax){
    if (!tree) return NULL;
    
    QueryResult *result = malloc(sizeof(QueryResult));
    if (!result) return NULL;
    
    unsigned int capacity = INITIAL_CAPACITY;
    result->indices = malloc(capacity * sizeof(int));
    if (!result->indices) {
        free(result);
        return NULL;
    }
    
    result->count = 0;
    AVLNodeRangeQuery(tree->root, latMin, latMax, longitudeArray, lonMin, lonMax, 
                         &result->indices, &result->count, &capacity);
    return result;
}

void DestroyQueryResult(QueryResult *result) {
    if (!result) return;
    free(result->indices);
    free(result);
}

int AVLTreeGetNodeHeight(AVLTree *tree) {
    return tree ? GetNodeHeight(tree->root) : 0;
}

bool AVLTreeIsEmpty(AVLTree *tree) {
    return !tree || !tree->root;
}

AVLTree* CreateAVLTreeFromArray(const float *values, int arraySize) {
    if (!values || arraySize <= 0) return NULL;
    
    AVLTree *tree = CreateAVLTree();
    if (!tree) return NULL;
    
    for (int i = 0; i < arraySize; i++) {
        if (!InsertAVLTree(tree, values[i], i)) {
            DestroyAVLTree(tree);
            return NULL;
        }
    }
    
    return tree;
}

bool AVLNodeValidateBalance(AVLNode *node) {
    if (!node) return true;
    int balance = GetNodeBalanceFactor(node);
    if (abs(balance) > 1) return false;
    return AVLNodeValidateBalance(node->left) && AVLNodeValidateBalance(node->right);
}

bool AVLTreeValidateBalance(AVLTree *tree) {
    if (!tree) return false;
    return AVLNodeValidateBalance(tree->root);
}

int AVLTreeGetHeight(AVLTree *tree) {
    return tree ? GetNodeHeight(tree->root) : 0;
}