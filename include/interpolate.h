#ifndef INTERPOLATE_H
#define INTERPOLATE_H

#include <stddef.h>
#include <stdint.h>
#include <math.h>
#include "geotransfer.h"
#include "data.h"
#include "rstartree.h"

typedef struct {
    double groundX, groundY, groundZ, groundH;
    double airX, airY, airZ, airH;
} CartesianInterpolator;

CartesianInterpolator calcInterParams(  const double groundX, const double groundY, const double groundZ, const double groundH,
                                        const double latitude, const double longitude, const double zeta);

Coordinate CalcCartesian(const CartesianInterpolator *interpolator, const float queryHeight);

bool GetGeodeticRange(GridInfo** const infoArray, const int lineCount, float *maxLatitude, float *minLatitude, float *maxLongitude, float *minLongitude);
bool InitClipGridArray(const HDFDataset* dataset, const int gridSize, const int initHeight, const int heightGap, const int heightCount, ClipGridResult* finalGrid);
float QueryClipMaxLongitude(const unsigned int leftLineIndex, const unsigned int rightLineIndex, const float minClipLatitude, const float maxClipLatitude, GridInfo** const infoArray);
unsigned int SearchLineIndex(const float latitude, unsigned int bias, GridInfo** const infoArray, unsigned int left, unsigned int right);
bool QueryBoundingBox(ClipGrid* clipGrid, GridInfo** const infoArray, const unsigned int lineCount);
double InterpolateValueIDW(const double queryPoint[3], const float queryHeight, const SpatialQueryResult* result, const float* valueArray, float power);
double InterpolateValueIDW_v(const unsigned int neightborCount, const double* distances, const int64_t* ids, const float* valueArray, const float power);
#endif