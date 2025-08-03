#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include "kdtree.h"

static int compare_by_latitude(const void* a, const void* b) {
    const KDCalcPoint* pa = (const KDCalcPoint*)a;
    const KDCalcPoint* pb = (const KDCalcPoint*)b;
    if (pa->latitude < pb->latitude) return -1;
    if (pa->latitude > pb->latitude) return 1;
    return 0;
}
static int compare_by_longitude(const void* a, const void* b) {
    const KDCalcPoint* pa = (const KDCalcPoint*)a;
    const KDCalcPoint* pb = (const KDCalcPoint*)b;
    if (pa->longitude < pb->longitude) return -1;
    if (pa->longitude > pb->longitude) return 1;
    return 0;
}

int SelectSplitDimension(KDCalcPoint* points, int count) {
    if (count <= 1) return 0;
    const double lat_variance = (points[count - 1].lat_square_sum - points[0].lat_square_sum) / count - (points[count - 1].lat_sum - points[0].lat_sum) * (points[count - 1].lat_sum - points[0].lat_sum) / count / count;
    const double lon_variance = (points[count - 1].lon_square_sum - points[0].lon_square_sum) / count - (points[count - 1].lon_sum - points[0].lon_sum) * (points[count - 1].lon_sum - points[0].lon_sum) / count / count;
    return (lat_variance > lon_variance) ? 0 : 1;
}

KDNode* BuildKDTree(KDCalcPoint* points, int count, int depth) {
    if (count <= 0) return NULL;
    int split_dim = SelectSplitDimension(points, count);
    if (split_dim == 0)
        qsort(points, count, sizeof(KDCalcPoint), compare_by_latitude);
    else
        qsort(points, count, sizeof(KDCalcPoint), compare_by_longitude);

    int median = count / 2;
    KDNode* node = (KDNode*)malloc(sizeof(KDNode));
    if (!node) return NULL;
    node->latitude = points[median].latitude;
    node->longitude = points[median].longitude;
    node->id = points[median].id;
    node->split_dim = split_dim;
    node->left = BuildKDTree(points, median, depth + 1);
    node->right = BuildKDTree(points + median + 1, count - median - 1, depth + 1);
    return node;
}

void DestroyKDNode(KDNode* node) {
    if (!node) return;
    DestroyKDNode(node->left);
    DestroyKDNode(node->right);
    free(node);
}

void DestroyKDTree(KDTree* tree) {
    if (!tree) return;
    DestroyKDNode(tree->root);
    free(tree);
}

KDNode* InsertKDNode(KDNode* node, float latitude, float longitude, int64_t id, int depth) {
    if (!node) {
        KDNode* new_node = (KDNode*)malloc(sizeof(KDNode));
        if (!new_node) return NULL;
        
        new_node->latitude = latitude;
        new_node->longitude = longitude;
        new_node->id = id;
        new_node->split_dim = depth % 2;
        new_node->left = new_node->right = NULL;
        return new_node;
    }
    
    if (node->split_dim == 0) { // split with latitude
        if (latitude < node->latitude)
            node->left = InsertKDNode(node->left, latitude, longitude, id, depth + 1);
        else
            node->right = InsertKDNode(node->right, latitude, longitude, id, depth + 1);
    } else {
        if (longitude < node->longitude)
            node->left = InsertKDNode(node->left, latitude, longitude, id, depth + 1);
        else
            node->right = InsertKDNode(node->right, latitude, longitude, id, depth + 1);
    }
    
    return node;
}

bool InsertKDTree(KDTree* tree, float latitude, float longitude, int64_t id) {
    if (!tree) return false;
    
    tree->root = InsertKDNode(tree->root, latitude, longitude, id, 0);
    if (tree->root) {
        tree->size++;
        return true;
    }
    return false;
}

double KDTreeSearchNodeWithinDistance(const KDNode* node, float queryLat, float queryLon, float distance) {
    if (!node) return INFINITY;
    
    double distSqr = (node->latitude - queryLat) * (node->latitude - queryLat) + (node->longitude - queryLon) * (node->longitude - queryLon);
    if (distSqr < distance * distance)
        return distSqr;
    
    KDNode* firstSubtree, *secondSubtree;
    double splitDist = INFINITY;
    
    if (node->split_dim == 0) { // split with latitude
        splitDist = queryLat - node->latitude;
        if (queryLat < node->latitude) {
            firstSubtree = node->left;
            secondSubtree = node->right;
        } else {
            firstSubtree = node->right;
            secondSubtree = node->left;
        }
    } else {
        splitDist = queryLon - node->longitude;
        if (queryLon < node->longitude) {
            firstSubtree = node->left;
            secondSubtree = node->right;
        } else {
            firstSubtree = node->right;
            secondSubtree = node->left;
        }
    }
    
    double firstSubtreeDist = KDTreeSearchNodeWithinDistance(firstSubtree, queryLat, queryLon, distance);

    // if the split distance is less than the current best distance, also search the other subtree
    if (firstSubtreeDist != INFINITY && splitDist * splitDist < firstSubtreeDist) {
        double secondSubtreeDist = KDTreeSearchNodeWithinDistance(secondSubtree, queryLat, queryLon, distance);
        return fmin(firstSubtreeDist, secondSubtreeDist);
    }

    return firstSubtreeDist;
}

bool KDTreeExistWithinDistance(const KDTree* tree, float queryLat, float queryLon, float distance) {
    if (!tree) return false;
    if (!tree->root) return false;
    double distSqr = KDTreeSearchNodeWithinDistance(tree->root, queryLat, queryLon, distance);
    return distSqr < distance * distance;
}
