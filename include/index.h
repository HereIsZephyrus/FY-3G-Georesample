#ifndef INDEX_H
#define INDEX_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct {
    int *indices;
    unsigned int size; // real size
    unsigned int capacity; // capacity
} IndexArray;

typedef struct AVLNode {
    float value;
    IndexArray *indices;
    int height;
    struct AVLNode *left;
    struct AVLNode *right;
} AVLNode;

typedef struct {
    AVLNode *root;
    int nodeCount;
} AVLTree;

typedef struct {
    int *indices;
    unsigned int count;
} QueryResult;

IndexArray* CreateIndexArray();
void DestroyIndexArray(IndexArray *arr);
bool AppendIndex(IndexArray *arr, int index);

int GetNodeHeight(AVLNode *node);
int GetNodeBalanceFactor(AVLNode *node);
void UpdateNodeHeight(AVLNode *node);
AVLNode* CreateAVLNode(float value, int arrayIndex);
void DestroyAVLNode(AVLNode *node);
AVLNode* InsertAVLNode(AVLNode *node, float value, int arrayIndex, bool *success);
AVLNode* RotateRight(AVLNode *y);
AVLNode* RotateLeft(AVLNode *x);

AVLTree* CreateAVLTree(void);
void DestroyAVLTree(AVLTree *tree);
bool InsertAVLTree(AVLTree *tree, float value, int arrayIndex);
AVLNode* SearchAVLTree(AVLTree *tree, float value);
AVLNode* SearchAVLNode(AVLNode *node, float value);

// Range query function
QueryResult* AVLTreeRangeQuery(AVLTree *tree, float latMin, float latMax, const float *longitudeArray, float lonMin, float lonMax);
void AVLNodeRangeQuery(AVLNode *node, float latMin, float latMax, const float *longitudeArray, float lonMin, float lonMax, int **result, unsigned int *count, unsigned int *capacity);
void DestroyQueryResult(QueryResult *result);

int AVLTreeGetHeight(AVLTree *tree);
bool AVLTreeIsEmpty(AVLTree *tree);

AVLTree* CreateAVLTreeFromArray(const float *values, int arraySize);

int AVLNodeGetHeight(AVLNode *node);
int AVLNodeGetBalanceFactor(AVLNode *node);
bool AVLTreeValidateBalance(AVLTree *tree);
bool AVLNodeValidateBalance(AVLNode *node);
#endif