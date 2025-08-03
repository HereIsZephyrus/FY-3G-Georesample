#include <string.h>
#include <stdlib.h>
#include "data.h"

const char* BAND_NAMES[2] = {"Ka", "Ku"};

void DestroyGridInfo(GridInfo* info) {
    if (!info) return;
    if (info->heightArray)
        free(info->heightArray);
    if (info->measuredArray)
        free(info->measuredArray);
}

void DestroyHDFDataset(HDFDataset* dataset){
    if (!dataset) return;
    for (int bandIndex = 0; bandIndex < 2; bandIndex++){
        for (unsigned int lineIndex = 0; lineIndex < dataset->globalAttribute.scanLineCount; lineIndex++){
            for (int angleIndex = 0; angleIndex < SCAN_ANGLE_COUNT; angleIndex++)
                DestroyGridInfo(&dataset->infoArray[bandIndex][lineIndex][angleIndex]);
            free(dataset->infoArray[bandIndex][lineIndex]);
        }
        free(dataset->infoArray[bandIndex]);
    }
}

void DestroyGeodeticGrid(GeodeticGrid* finalGrid){
    if (!finalGrid) return;
    for (int bandIndex = 0; bandIndex < 2; bandIndex++){
        if (finalGrid->latitudeArray[bandIndex])
            free(finalGrid->latitudeArray[bandIndex]);
        if (finalGrid->longitudeArray[bandIndex])
            free(finalGrid->longitudeArray[bandIndex]);
        if (finalGrid->elevationArray[bandIndex])
            free(finalGrid->elevationArray[bandIndex]);
        if (finalGrid->valueArray[bandIndex])
            free(finalGrid->valueArray[bandIndex]);
        if (finalGrid->validArray[bandIndex])
            free(finalGrid->validArray[bandIndex]);
    }
}

int getNumber(const char* str, int length){
    char temp[5];
    memset(temp, 0, 5);
    strncpy(temp, str, length);
    return atoi(temp);
}

DateTime CreateDateTime(const char* date, const char* time){
    DateTime dateTime;
    dateTime.year = getNumber(date, 4);
    dateTime.month = getNumber(date + 5, 2);
    dateTime.day = getNumber(date + 8, 2);
    dateTime.hour = getNumber(time, 2);
    dateTime.minute = getNumber(time + 3, 2);
    dateTime.second = getNumber(time + 6, 2);
    return dateTime;
}

char* ConstructDateTimeString(const DateTime* dateTime){
    char* dateTimeString = (char*)malloc(20 * sizeof(char));
    sprintf(dateTimeString, "%04d-%02d-%02dT%02d:%02d:%02d", dateTime->year, dateTime->month, dateTime->day, dateTime->hour, dateTime->minute, dateTime->second);
    return dateTimeString;
}

bool InitGeodeticGrid(GeodeticGrid* finalGrid, const int lineCount, const int heightCount){
    /**
    @brief Initialize the final grid
    @param finalGrid: the final grid to initialize
    @param lineCount: the line count
    @param heightCount: the height count
    @return true if successful, false otherwise
    */
    finalGrid->lineCount = lineCount;
    finalGrid->heightCount = heightCount;
    const int dims[3] = {finalGrid->lineCount, SCAN_ANGLE_COUNT, finalGrid->heightCount};
    const int memSize = dims[0] * dims[1] * dims[2] * sizeof(float);
    for (int bandIndex = 0; bandIndex < 2; bandIndex++){
        finalGrid->latitudeArray[bandIndex] = (float*)malloc(memSize);
        finalGrid->longitudeArray[bandIndex] = (float*)malloc(memSize);
        finalGrid->elevationArray[bandIndex] = (float*)malloc(memSize);
        finalGrid->valueArray[bandIndex] = (float*)malloc(memSize);
        finalGrid->validArray[bandIndex] = (bool*)malloc(memSize);
        if (!finalGrid->latitudeArray[bandIndex] || !finalGrid->longitudeArray[bandIndex] || !finalGrid->elevationArray[bandIndex] || !finalGrid->valueArray[bandIndex] || !finalGrid->validArray[bandIndex]){
            fprintf(stderr, "Failed to allocate memory for latitudeArray, longitudeArray, elevationArray or valueArray\n");
            return false;
        }
        memset(finalGrid->validArray[bandIndex], true, memSize);
    }
    return true;
}

void DestroyClipGridResult(ClipGridResult* clipGridResult){
    if (!clipGridResult) return;
    for (int bandIndex = 0; bandIndex < 2; bandIndex++){
        if (clipGridResult->clipGrids[bandIndex]){
            for (unsigned int clipIndex = 0; clipIndex < clipGridResult->clipCount; clipIndex++)
                if (clipGridResult->clipGrids[bandIndex][clipIndex].value)
                    free(clipGridResult->clipGrids[bandIndex][clipIndex].value);
            if (clipGridResult->clipGrids[bandIndex])
                free(clipGridResult->clipGrids[bandIndex]);
        }
    }
}