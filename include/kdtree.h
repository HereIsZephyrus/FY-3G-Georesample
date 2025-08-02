#ifndef KDTREE_H
#define KDTREE_H
#include <stdint.h>
#include <stdbool.h>

typedef struct KDNode{
    float latitude, longitude;
    int64_t id;
    struct KDNode *left, *right;
    int split_dim; // 0 for latitude, 1 for longitude
} KDNode;

typedef struct KDTree{
    KDNode* root;
    unsigned int size;
    unsigned int heightIndex;
} KDTree;

typedef struct{
    float latitude, longitude;
    double lat_sum, lon_sum;
    double lat_square_sum, lon_square_sum;
    int64_t id;
} KDCalcPoint;

int SelectSplitDimension(KDCalcPoint* points, int count);
KDNode* BuildKDTree(KDCalcPoint* points, int count, int depth);
void DestroyKDTree(KDTree* tree);
void DestroyKDNode(KDNode* node);
bool InsertKDTree(KDTree* tree, float latitude, float longitude, int64_t id);
KDNode* InsertKDNode(KDNode* node, float latitude, float longitude, int64_t id, int depth);
double KDTreeSearchNodeWithinDistance(KDNode* node, float queryLat, float queryLon, float distance);
bool KDTreeExistWithinDistance(KDTree* tree, float queryLat, float queryLon, float distance);
#endif // KDTREE_H