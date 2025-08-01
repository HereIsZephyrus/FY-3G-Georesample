#ifndef INTERPOLATE_H
#define INTERPOLATE_H
#define DEFAULT_GRID_SIZE 5000 // 5000m
#define DEFAULT_MINIMAL_HEIGHT 100 // 100m
#define DEFAULT_HEIGHT_GAP 200 // 200m
#define DEFAULT_HEIGHT_COUNT 60

#include <stddef.h>
#include <stdint.h>
#include <math.h>
#include "geotransfer.h"
#include "data.h"
#include "index.h"

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
//double InterpolateValueIDWBatch(const double queryPoint[3], const float queryHeight, const SpatialQueryResult* result, const float* valueArray, const RStarPoint* points, float power);
#endif