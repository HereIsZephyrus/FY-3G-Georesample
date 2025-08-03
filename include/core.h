#ifndef CORE_H
#define CORE_H

#include "data.h"
#include "index.h"
#include "kdtree.h"

bool ProcessDataset(const HDFDataset* dataset, GeodeticGrid* geodeticGrid, PointBatch* pointBatch);
void CalculateGridData(const GridInfo* dataset, GeodeticGrid* geodeticGrid, PointBatch* pointBatch, unsigned int bandIndex, unsigned int lineIndex, unsigned int angleIndex);
bool InterpolateGrid(const GeodeticGrid* processedGrid, const PointBatch* pointBatch, IndexForest* forest, ClipGridResult* finalGrid);
bool InitClipResult(const HDFDataset* dataset, const PointBatch* pointBatch, IndexForest* forest, ClipGridResult* finalGrid);
bool InterpolateClipGrid(const RStarPoint* points, KDTree** flatindexTree, RStarIndex* indexTree, const float* valueArray, ClipGrid* clipGrid);
bool InterpolateClipGridBatch(const RStarPoint* points, RStarIndex* indexTree, KDTree** flatindexForest, const float* valueArray, ClipGrid* clipGrid);
#endif