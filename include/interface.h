#ifndef INTERFACE_H
#define INTERFACE_H
#include <hdf5.h>
#define SCAN_ANGLE_COUNT 59
#define SCAN_HEIGHT_COUNT 500
#define GEOLOCATION_GROUP_NAME "Geolocation"
#define PRE_GROUP_NAME "PRE"

typedef struct {
    double groundL, groundB, groundH;
    double airL, airB, zeta;
    double evaluation, clutterFreeBottomIndex;
    unsigned int nx, ny;
    double *heightArray, *measuredArray;
} GridInfo;

typedef struct{
    unsigned int year, month, day, hour, minute, second;
} DateTime;

typedef struct {
    unsigned int scanLineCount;
    DateTime startDateTime, endDateTime;
} HDFGlobalAttribute;

GridInfo* CreateGridInfo(unsigned int nx, unsigned int ny);
void DestroyGridInfo(GridInfo* info);

DateTime CreateDateTime(const char* date, const char* time);

bool ReadHDF5(const char* filename);
bool ReadSingleAttribute(hid_t fileID, const char* attributeName, hid_t typeID, void* buffer);
bool ReadGlobalAttribute(hid_t fileID, HDFGlobalAttribute* globalAttribute);
bool ReadBand(hid_t fileID, hid_t geolocationID, hid_t preID, const char* bandName);
const char* ConstructPath(const char* pathNames[], const int pathLength);
#endif