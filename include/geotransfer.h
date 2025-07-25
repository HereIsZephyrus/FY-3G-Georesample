#ifndef GEOTRANSFER_H
#define GEOTRANSFER_H
#include <math.h>
#include <stdbool.h>

#define WGS84_A 6378137.0
#define WGS84_B 6356752.3142
#define WGS84_E (sqrt(WGS84_A * WGS84_A - WGS84_B * WGS84_B) / WGS84_A)
#define M_PI 3.14159265358979323846

typedef struct Coordinate{
    double x, y, z;
    double l, b, h;
} Coordinate;

double ComputeN(const double latitude);
double ComputeS(const double t1, const double t2, const double t3, const double t4, const double e, const double r);
double ToRadians(const double degree);
double ToDegrees(const double radians);
bool IsGeodeticValid(const double latitude, const double longitude, const double height);
bool TransferGeodeticToCartesian(const double latitude, const double longitude, const double height, double *x, double *y, double *z);
Coordinate TransferCartesianToGeodetic(const double x, const double y, const double z, const bool iterative);
void TransferCartesianToGeodeticLagrange(const double x, const double y, const double z, double *latitude, double *height);
void TransferCartesianToGeodeticIterative(const double x, const double y, const double z, double *latitude, double *height);

#endif