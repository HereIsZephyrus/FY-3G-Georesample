#include "geotransfer.h"
#include <stdio.h>
#define B_ITER_TOLERANCE 1e-10
#define B_ITER_MAX_ITER 100

double ComputeN(const double latitude) {
    /**
     * @brief compute normal radius of curvature
     * @param latitude: latitude in radians
     * @return normal radius of curvature
    */
    return WGS84_A / sqrt(1 - WGS84_E * WGS84_E * sin(latitude) * sin(latitude));
}

double ComputeS(const double t1, const double t2, const double t3, const double t4, const double e, const double r) {
    /**
     * @brief compute s
     * @param t1, t2, t3, t4: intermediate variables
     * @param e: eccentricity
     * @param r: radius
     * @return s
    */
    double s = t1
        - (3 * pow(e, 4) / (2 * pow(r, 2))) * pow(t1, 3) * pow(t4, 2)
        + (pow(e, 6) / (2 * pow(r, 3))) * pow(t1, 3) * pow(t2, 2) * (4 * pow(t3, 2) - pow(t4, 2))
        + (pow(e, 8) / (2 * pow(r, 4))) * pow(t1, 3) * pow(t4, 4) * (5 * pow(t1, 2) + pow(t2, 2))
        + (5 * pow(e, 8) / (8 * pow(r, 4))) * pow(t1, 5) * pow(t2, 2) * (3 * pow(t4, 2) - 4 * pow(t3, 2))
        - (10 * pow(e, 10) / pow(r, 5)) * pow(t1, 7) * pow(t4, 4)
        - (5 * pow(e, 12) / (8 * pow(r, 6))) * pow(t1, 5) * pow(t4, 6) * (7 * pow(t1, 2) + 3 * pow(t2, 2))
        + (3 * pow(e, 10) / (8 * pow(r, 5))) * pow(t1, 5) * pow(t2, 2) * (8 * pow(t1, 2) * pow(t3, 2) - 12 * pow(t1, 2) * pow(t4, 2) + pow(t2, 2) * pow(t4, 2))
        - (3 * pow(e, 12) / (8 * pow(r, 6))) * pow(t1, 5) * pow(t2, 4) * (pow(t4, 4) + 25 * pow(t3, 2) * pow(t4, 2) - 68 * pow(t3, 4))
        - (3 * pow(e, 14) / (8 * pow(r, 7))) * pow(t1, 5) * pow(t2, 4) * pow(t4, 2) * (pow(t4, 4) - 23 * pow(t3, 2) * pow(t4, 2) - 92 * pow(t3, 4))
        + (3 * pow(e, 16) / (8 * pow(r, 8))) * pow(t1, 5) * pow(t2, 4) * pow(t4, 4) * (pow(t4, 4) + 14 * pow(t3, 2) * pow(t4, 2) + 21 * pow(t3, 4));
    return s;
}

double ToRadians(const double degree){return degree * M_PI / 180;}
double ToDegrees(const double radians){return radians * 180 / M_PI;}

bool IsGeodeticValid(const double latitude, const double longitude, const double height) {
    /**
     * @brief check if the geodetic coordinates are valid
     * @param latitude: latitude in radians
     * @param longitude: longitude in radians
     * @param height: height in meters
     * @return true if valid, false if invalid
    */
    if (latitude < -90 || latitude > 90){
        fprintf(stderr, "Latitude out of range: %f\n", latitude);
        return false;
    }
    if (longitude < -180 || longitude > 180){
        fprintf(stderr, "Longitude out of range: %f\n", longitude);
        return false;
    }
    if (height < -100 || height > 200000){
        fprintf(stderr, "Height out of range: %f\n", height);
        return false;
    }
    return true;
}

bool TransferGeodeticToCartesian(const double latitude, const double longitude, const double height, double *x, double *y, double *z) {
    /**
     * @brief transfer geodetic to cartesian coordinates
     * @param latitude, longitude, height: geodetic coordinates
     * @param x, y, z: cartesian coordinates
     * @return true if success, false if failed
    */
    if (!IsGeodeticValid(latitude, longitude, height))
        return false;
    const double latitude_rad = ToRadians(latitude);
    const double longitude_rad = ToRadians(longitude);
    const double N = ComputeN(latitude_rad);
    *x = (N + height) * cos(latitude_rad) * cos(longitude_rad);
    *y = (N + height) * cos(latitude_rad) * sin(longitude_rad);
    *z = (N * (1 - WGS84_E * WGS84_E) + height) * sin(latitude_rad);
    return true;
}

void TransferCartesianToGeodeticLagrange(const double x, const double y, const double z, double *latitude, double *height) {
    /**
     * @brief transfer cartesian to geodetic coordinates using lagrange method
     * @param x, y, z: cartesian coordinates
     * @param latitude, height: geodetic coordinates
    */
    const double R = sqrt(x * x + y * y + (1 - WGS84_E * WGS84_E) * z * z);
    const double tdenominator = R - WGS84_E * WGS84_E * WGS84_A * (x * x + y * y)/(R*R);
    if (tdenominator == 0) {
        fprintf(stderr, "tdenominator is 0\n");
        return;
    }
    const double t1 = sqrt((1 - WGS84_E * WGS84_E)) * z / tdenominator;
    const double t2 = sqrt(x * x + y * y) / tdenominator;
    const double t3 = sqrt(1 - WGS84_E * WGS84_E) * z / R;
    const double t4 = sqrt(x * x + y * y) / R;
    const double s = ComputeS(t1, t2, t3, t4, WGS84_E, R / WGS84_A);
    *latitude = ToDegrees(asin(s / sqrt(1 - WGS84_E * WGS84_E + WGS84_E * WGS84_E * s * s)));
    *height = sqrt(pow(sqrt(x * x + y * y) - WGS84_A * sqrt(1 - s * s), 2) + pow(z - WGS84_B * s, 2));
}

void TransferCartesianToGeodeticIterative(const double x, const double y, const double z, double *latitude, double *height) {
    /**
     * @brief transfer cartesian to geodetic coordinates using iterative method
     * @param x, y, z: cartesian coordinates
     * @param latitude, height: geodetic coordinates
    */
    const double xy_hypot = sqrt(x * x + y * y);
    double lat0 = 0.0;
    *latitude = atan(z / xy_hypot);
    double iter = 0;
    do {
        lat0 = *latitude;
        double N = ComputeN(lat0);
        *latitude = atan((z + WGS84_E * WGS84_E * N * sin(lat0)) / xy_hypot);
    } while (fabs(*latitude - lat0) > B_ITER_TOLERANCE && iter++ < B_ITER_MAX_ITER);
    double N = ComputeN(*latitude);
    if (fabs(*latitude) < M_PI / 4.0) {
        double R = sqrt(x * x + y * y + z * z);
        double phi = atan(z / xy_hypot);
        *height = R * cos(phi) / cos(*latitude) - N;
    } else
        *height = z / sin(*latitude) - N * (1 - WGS84_E * WGS84_E);
    *latitude = ToDegrees(*latitude);
}

Coordinate TransferCartesianToGeodetic(const double x, const double y, const double z, const bool iterative) {
    /**
     * @brief transfer cartesian to geodetic coordinates
     * @param iterative: use iterative method(true) or use lagrange method(false)
     * @param x, y, z: cartesian coordinates
     * @param latitude, height: geodetic coordinates
     * @return true if success, false if failed
    */
    double longitude = ToDegrees(atan2(y, x)), latitude = 0.0, height = 0.0;
    
    if (iterative)
        TransferCartesianToGeodeticIterative(x, y, z, &latitude, &height);
    else
        TransferCartesianToGeodeticLagrange(x, y, z, &latitude, &height);

    if (!IsGeodeticValid(latitude, longitude, height)){
        fprintf(stderr, "geodetic coordinates is not valid\n");
        return (Coordinate){0, 0, 0, 0, 0, 0};
    }
    return (Coordinate){x, y, z, latitude, longitude, height};
}