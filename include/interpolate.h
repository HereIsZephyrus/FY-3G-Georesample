#ifndef INTERPOLATE_H
#define INTERPOLATE_H
#define DEFAULT_HEIGHT_COUNT 60

#include <stddef.h>
#include <stdint.h>
#include <math.h>
#include "geotransfer.h"
#include "data.h"

typedef struct {
    double groundX, groundY, groundZ, groundH;
    double airX, airY, airZ, airH;
} CartesianInterpolator;

CartesianInterpolator calcInterParams(  const double groundX, const double groundY, const double groundZ, const double groundH,
                                        const double latitude, const double longitude, const double zeta);

Coordinate calcCartesian(const CartesianInterpolator *interpolator, const float queryHeight);

bool GetGeodeticRange(GridInfo** const infoArray, const int lineCount, float *maxLatitude, float *minLatitude, float *maxLongitude, float *minLongitude);
bool InitClipGridArray(const HDFDataset* dataset, const int gridSize, const int initHeight, const int heightGap, const int heightCount, ClipGridResult* finalGrid);
float QueryClipMaxLongitude(const float minClipLatitude, const float maxClipLatitude, GridInfo** const infoArray, const int lineCount);
#endif