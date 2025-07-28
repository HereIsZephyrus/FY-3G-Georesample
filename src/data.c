#include <string.h>
#include <stdlib.h>
#include "data.h"

const char* BAND_NAMES[2] = {"Ka", "Ku"};

void DestroyGridInfo(GridInfo* info) {
    /**
    @brief Destroy GridInfo
    @param info: the GridInfo to destroy
    */
    if (info) {
        if (info->heightArray)
            free(info->heightArray);
        if (info->measuredArray)
            free(info->measuredArray);
    }
}

void DestroyHDFDataset(HDFDataset* dataset){
    for (int bandIndex = 0; bandIndex < 2; bandIndex++){
        for (unsigned int lineIndex = DEBUG_INDEX; lineIndex < dataset->globalAttribute.scanLineCount; lineIndex++){
            for (int angleIndex = 0; angleIndex < SCAN_ANGLE_COUNT; angleIndex++)
                DestroyGridInfo(&dataset->infoArray[bandIndex][lineIndex][angleIndex]);
            free(dataset->infoArray[bandIndex][lineIndex]);
        }
        free(dataset->infoArray[bandIndex]);
    }
}

void DestroyFinalGrid(FinalGrid* finalGrid){
    for (int bandIndex = 0; bandIndex < 2; bandIndex++){
        free(finalGrid->latitudeArray[bandIndex]);
        free(finalGrid->longitudeArray[bandIndex]);
        free(finalGrid->elevationArray[bandIndex]);
        free(finalGrid->valueArray[bandIndex]);
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