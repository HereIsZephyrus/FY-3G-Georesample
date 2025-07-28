#ifndef CORE_H
#define CORE_H

#include "data.h"

bool ProcessDataset(const HDFDataset* dataset, FinalGrid* finalGrid);
bool InitFinalGrid(const HDFDataset* dataset, FinalGrid* finalGrid);
void CalculateInterpolation(const HDFDataset* dataset, FinalGrid* finalGrid, int bandIndex, unsigned int lineIndex, unsigned int angleIndex);
#endif