#ifndef CORE_H
#define CORE_H

#include "data.h"
#include "index.h"

bool ProcessDataset(const HDFDataset* dataset, GeodeticGrid* geodeticGrid, RStarPointBatch* pointBatch);
void CalculateGridData(const GridInfo* dataset, GeodeticGrid* geodeticGrid, RStarPointBatch* pointBatch, unsigned int bandIndex, unsigned int lineIndex, unsigned int angleIndex);
bool CreateRStarTree(RStarPointBatch* pointBatch, RStarTree* tree);
bool InterpolateGrid(const HDFDataset* dataset, const GeodeticGrid* processedGrid, const RStarTree* tree, ClipGridResult* finalGrid);
//bool Interpolate(const HDFDataset* dataset, const GeodeticGrid* processedGrid, AVLTree* indexTree[2], ClipGridResult* finalGrid);
//bool InterpolateClipGrid(AVLTree* indexTree, const float* longitudeArray, const float* valueArray, ClipGrid* clipGrid);
#endif