#ifndef INTERFACE_H
#define INTERFACE_H
#include <hdf5.h>
#define SCAN_ANGLE_COUNT 59
#define SCAN_HEIGHT_COUNT 500
#define GEOLOCATION_GROUP_NAME "Geolocation"
#define PRE_GROUP_NAME "PRE"

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
} HDFGlobalAttribute;

typedef struct {
    hid_t elevationID, latitudeID, longitudeID, zenithID, heightID, groundHeightID, valueID, binClutterID;
} HDFBandRequired;

GridInfo* CreateGridInfo(unsigned int nx, unsigned int ny);
void DestroyGridInfo(GridInfo* info);

DateTime CreateDateTime(const char* date, const char* time);

int getNumber(const char* str, int length);
bool ReadHDF5(const char* filename);
bool ReadSingleAttribute(hid_t fileID, const char* attributeName, hid_t typeID, void* buffer);
bool ReadGlobalAttribute(hid_t fileID, HDFGlobalAttribute* globalAttribute);
hid_t GetDatasetID(hid_t fileID, const char* path);
bool GetRequiredDatasetID(hid_t fileID, const char* bandName, HDFBandRequired* required);
bool ReadBand(hid_t fileID, const char* bandName, HDFGlobalAttribute* globalAttribute);
bool ReadSingleScanLine(int lineIndex, const HDFBandRequired* required, GridInfo* infoLine);
const char* ConstructPath(const char* pathNames[], const int pathLength);
#endif