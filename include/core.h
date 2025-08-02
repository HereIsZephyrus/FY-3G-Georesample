#ifndef CORE_H
#define CORE_H

#include "data.h"
#include "index.h"
#include "kdtree.h"

bool ProcessDataset(const HDFDataset* dataset, GeodeticGrid* geodeticGrid, RStarPointBatch* pointBatch);
void CalculateGridData(const GridInfo* dataset, GeodeticGrid* geodeticGrid, RStarPointBatch* pointBatch, unsigned int bandIndex, unsigned int lineIndex, unsigned int angleIndex);
bool InterpolateGrid(const GeodeticGrid* processedGrid, const RStarPointBatch* pointBatch, IndexForest* forest, ClipGridResult* finalGrid);
bool InitClipResult(const HDFDataset* dataset, const RStarPointBatch* pointBatch, IndexForest* forest, ClipGridResult* finalGrid);
bool IsValidHeightData(const float coordinateHeight, const float elevation, const unsigned int heightIndex, const float clutterFreeBottomIndex);
bool InterpolateClipGrid(const RStarPoint* points, KDTree** flatindexTree, RStarIndex* indexTree, const float* valueArray, ClipGrid* clipGrid);
#endif