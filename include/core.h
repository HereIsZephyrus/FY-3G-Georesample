#ifndef CORE_H
#define CORE_H

#include "data.h"
#include "index.h"

bool ProcessDataset(const HDFDataset* dataset, GeodeticGrid* geodeticGrid, RStarPointBatch* pointBatch);
void CalculateGridData(const GridInfo* dataset, GeodeticGrid* geodeticGrid, RStarPointBatch* pointBatch, unsigned int bandIndex, unsigned int lineIndex, unsigned int angleIndex);
bool CreateRStarForest(const RStarPointBatch* pointBatch, ClipGridResult* finalGrid, RStarForest* forest);
bool InterpolateGrid(const HDFDataset* dataset, const GeodeticGrid* processedGrid, RStarForest* forest, ClipGridResult* finalGrid);
bool InitClipResult(const HDFDataset* dataset, const RStarPointBatch* pointBatch, RStarForest* forest, ClipGridResult* finalGrid);
bool IsValidHeightData(const float coordinateHeight, const float elevation, const unsigned int heightIndex, const float clutterFreeBottomIndex);
//bool Interpolate(const HDFDataset* dataset, const GeodeticGrid* processedGrid, AVLTree* indexTree[2], ClipGridResult* finalGrid);
//bool InterpolateClipGrid(AVLTree* indexTree, const float* longitudeArray, const float* valueArray, ClipGrid* clipGrid);
#endif