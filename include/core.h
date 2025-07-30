#ifndef CORE_H
#define CORE_H

#include "data.h"
#include "index.h"

bool ProcessDataset(const HDFDataset* dataset, GeodeticGrid* finalGrid);
void CalculateGridData(const GridInfo* dataset, GeodeticGrid* finalGrid, unsigned int bandIndex, unsigned int lineIndex, unsigned int angleIndex);
bool Interpolate(const HDFDataset* dataset, const GeodeticGrid* processedGrid, AVLTree* indexTree[2], ClipGridResult* finalGrid);
bool InterpolateClipGrid(AVLTree* indexTree, const float* longitudeArray, const float* valueArray, ClipGrid* clipGrid);
#endif