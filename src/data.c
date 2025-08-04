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
    for (unsigned int lineIndex = 0; lineIndex < dataset->globalAttribute.scanLineCount; lineIndex++){
        for (int angleIndex = 0; angleIndex < SCAN_ANGLE_COUNT; angleIndex++)
            DestroyGridInfo(&dataset->infoArray[lineIndex][angleIndex]);
        free(dataset->infoArray[lineIndex]);
    }
    free(dataset->infoArray);
}

void DestroyGeodeticGrid(GeodeticGrid* finalGrid){
    if (!finalGrid) return;
    if (finalGrid->latitudeArray)
        free(finalGrid->latitudeArray);
    if (finalGrid->longitudeArray)
        free(finalGrid->longitudeArray);
    if (finalGrid->elevationArray)
        free(finalGrid->elevationArray);
    if (finalGrid->valueArray)
        free(finalGrid->valueArray);
    if (finalGrid->validArray)
        free(finalGrid->validArray);
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
    finalGrid->latitudeArray = (float*)malloc(memSize);
    finalGrid->longitudeArray = (float*)malloc(memSize);
    finalGrid->elevationArray = (float*)malloc(memSize);
    finalGrid->valueArray = (float*)malloc(memSize);
    finalGrid->validArray = (bool*)malloc(memSize);
    if (!finalGrid->latitudeArray || !finalGrid->longitudeArray || !finalGrid->elevationArray || !finalGrid->valueArray || !finalGrid->validArray){
        fprintf(stderr, "Failed to allocate memory for latitudeArray, longitudeArray, elevationArray or valueArray\n");
        return false;
    }
    memset(finalGrid->validArray, true, memSize);
    return true;
}

void DestroyClipGridResult(ClipGridResult* clipGridResult){
    if (!clipGridResult) return;
    for (unsigned int clipIndex = 0; clipIndex < clipGridResult->clipCount; clipIndex++)
        if (clipGridResult->clipGrids[clipIndex].value)
            free(clipGridResult->clipGrids[clipIndex].value);
    if (clipGridResult->clipGrids)
        free(clipGridResult->clipGrids);
}