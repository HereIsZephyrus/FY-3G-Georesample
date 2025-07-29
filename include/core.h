#ifndef CORE_H
#define CORE_H

#include "data.h"

bool ProcessDataset(const HDFDataset* dataset, GeodeticGrid* finalGrid);
void CalculateGridData(const GridInfo* dataset, GeodeticGrid* finalGrid, unsigned int bandIndex, unsigned int lineIndex, unsigned int angleIndex);
#endif