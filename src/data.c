#include "data.h"
const char* BAND_NAMES[2] = {"Ka", "Ku"};

void DestroyGridInfo(GridInfo* info) {
    /**
    @brief Destroy GridInfo
    @param info: the GridInfo to destroy
    */
    if (info) {
        free(info->heightArray);
        free(info->measuredArray);
    }
}

void DestroyHDFDataset(HDFDataset* dataset){
    for (int bandIndex = 0; bandIndex < 2; bandIndex++){
        for (unsigned int lineIndex = 0; lineIndex < dataset->globalAttribute.scanLineCount; lineIndex++){
            for (int angleIndex = 0; angleIndex < SCAN_ANGLE_COUNT; angleIndex++)
                DestroyGridInfo(&dataset->infoArray[bandIndex][lineIndex][angleIndex]);
            free(dataset->infoArray[bandIndex][lineIndex]);
        }
        free(dataset->infoArray[bandIndex]);
    }
}

void DestroyFinalGrid(FinalGrid* finalGrid){
    for (unsigned int lineIndex = 0; lineIndex < finalGrid->lineCount; lineIndex++){
        for (unsigned int angleIndex = 0; angleIndex < SCAN_ANGLE_COUNT; angleIndex++){
            free(finalGrid->latitudeArray[lineIndex][angleIndex]);
            free(finalGrid->longitudeArray[lineIndex][angleIndex]);
            free(finalGrid->elevationArray[lineIndex][angleIndex]);
            free(finalGrid->valueArray[lineIndex][angleIndex]);
        }
        free(finalGrid->latitudeArray[lineIndex]);
        free(finalGrid->longitudeArray[lineIndex]);
        free(finalGrid->elevationArray[lineIndex]);
        free(finalGrid->valueArray[lineIndex]);
    }
    free(finalGrid->latitudeArray);
    free(finalGrid->longitudeArray);
    free(finalGrid->elevationArray);
    free(finalGrid->valueArray);
}