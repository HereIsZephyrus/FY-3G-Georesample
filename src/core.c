#include "core.h"

bool InitFinalGrid(const HDFDataset* dataset, FinalGrid* finalGrid){
    /**
    @brief Initialize the final grid
    @param finalGrid: the final grid to initialize
    @return true if successful, false otherwise
    */
    finalGrid->lineCount = dataset->globalAttribute.scanLineCount;
    finalGrid->heightCount = SCAN_HEIGHT_COUNT / 10;
    const int dims[3] = {finalGrid->lineCount, SCAN_ANGLE_COUNT, finalGrid->heightCount};
    const int memSize = dims[0] * dims[1] * dims[2] * sizeof(float);
    for (int bandIndex = 0; bandIndex < 2; bandIndex++){
        finalGrid->latitudeArray[bandIndex] = (float*)malloc(memSize);
        finalGrid->longitudeArray[bandIndex] = (float*)malloc(memSize);
        finalGrid->elevationArray[bandIndex] = (float*)malloc(memSize);
        finalGrid->valueArray[bandIndex] = (float*)malloc(memSize);
        if (!finalGrid->latitudeArray[bandIndex] || !finalGrid->longitudeArray[bandIndex] || !finalGrid->elevationArray[bandIndex] || !finalGrid->valueArray[bandIndex]){
            fprintf(stderr, "Failed to allocate memory for latitudeArray, longitudeArray, elevationArray or valueArray\n");
            return false;
        }
    }
    return true;
}

void CalculateInterpolation(const HDFDataset* dataset, FinalGrid* finalGrid, int bandIndex, unsigned int lineIndex, unsigned int angleIndex){
    /**
    @brief Calculate the interpolation
    @param dataset: the dataset to calculate the interpolation
    @param finalGrid: the final grid to store the data
    @param bandIndex: the band index
    @param lineIndex: the line index
    @param angleIndex: the angle index
    */
    for (unsigned int heightIndex = 0; heightIndex < finalGrid->heightCount; heightIndex++){
        const int index = lineIndex * SCAN_ANGLE_COUNT * finalGrid->heightCount + angleIndex * finalGrid->heightCount + heightIndex;
        const int sampleLineIndex = DEBUG_INDEX;
        finalGrid->latitudeArray[bandIndex][index] = dataset->infoArray[bandIndex][sampleLineIndex][angleIndex].groundB;
        finalGrid->longitudeArray[bandIndex][index] = dataset->infoArray[bandIndex][sampleLineIndex][angleIndex].groundL;
        finalGrid->elevationArray[bandIndex][index] = dataset->infoArray[bandIndex][sampleLineIndex][angleIndex].evaluation;
        finalGrid->valueArray[bandIndex][index] = dataset->infoArray[bandIndex][sampleLineIndex][angleIndex].measuredArray[heightIndex];
    }
}

bool ProcessDataset(const HDFDataset* dataset, FinalGrid* finalGrid){
    /**
    @brief Construct final grid
    @param dataset: the dataset to construct the final grid
    @param finalGrid: the final grid to store the data
    @return true if successful, false otherwise
    */
    if (!InitFinalGrid(dataset, finalGrid)){
        fprintf(stderr, "Failed to initialize final grid\n");
        return false;
    }
    for (int bandIndex = 0; bandIndex < 2; bandIndex++){
        #pragma omp parallel for shared(dataset, finalGrid, bandIndex)
        for (unsigned int lineIndex = 0; lineIndex < finalGrid->lineCount; lineIndex++)
            for (unsigned int angleIndex = 0; angleIndex < SCAN_ANGLE_COUNT; angleIndex++)
                CalculateInterpolation(dataset, finalGrid, bandIndex, lineIndex, angleIndex);
    }
    return true;
}