#ifndef DATA_H
#define DATA_H
#define SCAN_ANGLE_COUNT 59
#define SCAN_HEIGHT_COUNT 500
#define GEOLOCATION_GROUP_NAME "Geolocation"
#define PRE_GROUP_NAME "PRE"
#include <stdlib.h>
#include <hdf5.h>

typedef struct {
    float groundL, groundB, groundH;
    float airL, airB, zeta;
    float evaluation, clutterFreeBottomIndex;
    unsigned int lineIndex, angleIndex;
    float *heightArray, *measuredArray;
} GridInfo;

typedef struct{
    unsigned int year, month, day, hour, minute, second;
} DateTime;

typedef struct {
    unsigned int scanLineCount;
    DateTime startDateTime, endDateTime;
    bool ascending;
} HDFGlobalAttribute;

extern const char* BAND_NAMES[2];
typedef struct {    
    GridInfo** infoArray[2];
    HDFGlobalAttribute globalAttribute;
} HDFDataset;

typedef struct {
    unsigned int lineCount, heightCount;
    float *latitudeArray[2], *longitudeArray[2], *elevationArray[2], *valueArray[2]; // [bandIndex][[lineCount][angleCount][heightCount]]
    bool *validArray[2]; // [bandIndex][[lineCount][angleCount][heightCount]]
} GeodeticGrid;

typedef struct {
    unsigned int latitudeCount, longitudeCount, heightCount;
    unsigned int leftLineIndex, rightLineIndex;
    float maxLatitude, minLatitude, maxLongitude, minLongitude, minHeight;
    float latitudeGap, longitudeGap, heightGap;
    float *value; // [latitudeCount][longitudeCount][heightCount]
} ClipGrid;

typedef struct{
    unsigned int clipCount;
    ClipGrid* clipGrids[2];
    HDFGlobalAttribute globalAttribute;
} ClipGridResult;

DateTime CreateDateTime(const char* date, const char* time);
int getNumber(const char* str, int length);
char* ConstructDateTimeString(const DateTime* dateTime);
bool InitGeodeticGrid(GeodeticGrid* finalGrid, const int lineCount, const int heightCount);

void DestroyGridInfo(GridInfo* info);
void DestroyHDFDataset(HDFDataset* dataset);
void DestroyGeodeticGrid(GeodeticGrid* finalGrid);
void DestroyClipGridResult(ClipGridResult* clipGridResult);
#endif