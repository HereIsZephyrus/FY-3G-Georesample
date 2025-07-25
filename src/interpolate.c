#include <stdio.h>
#include <stdlib.h>
#include "interpolate.h"
#include "geotransfer.h"

CartesianInterpolator calcInterParams(  const double groundX, const double groundY, const double groundZ, const double groundH,
                                        const double latitude, const double longitude, const double zeta) {
    const double N = ComputeN(latitude);
    const double zeta_rad = ToRadians(zeta), latitude_rad = ToRadians(latitude), longitude_rad = ToRadians(longitude);
    const double alpha = 1 - 1 / (cos(zeta_rad) * cos(zeta_rad));
    const double beta = 2 * groundZ / (cos(zeta_rad) * cos(zeta_rad)) +
                        2 * N * (cos(latitude_rad) * cos(latitude_rad) -
                        2 * (groundX * cos(latitude_rad) * cos(longitude_rad) + 
                             groundY * cos(latitude_rad) * cos(longitude_rad) + 
                             groundZ * sin(latitude_rad)));
    const double gamma = groundX * groundX + groundY * groundY + groundZ * groundZ +
                        N * N * (1 - 2 * WGS84_E * WGS84_E * sin(latitude_rad) * sin(latitude_rad) +
                                WGS84_E * WGS84_E * WGS84_E * WGS84_E * sin(latitude_rad) * sin(latitude_rad)) -
                        2 * (groundX * N * cos(latitude_rad) * cos(longitude_rad) +
                            groundY * N * cos(latitude_rad) * cos(longitude_rad) +
                            groundZ * N * (1 - WGS84_E * WGS84_E) * sin(latitude_rad));
    const double delta = beta * beta - 4 * alpha * gamma;
    if (delta < 0) {
        fprintf(stderr, "delta is less than 0\n");
        return (CartesianInterpolator){0, 0, 0, 0, 0, 0, 0, 0};
    }
    double airX, airY, airZ, airH;
    airH = (-beta + sqrt(delta)) / (2 * alpha);
    TransferGeodeticToCartesian(latitude, longitude, airH, &airX, &airY, &airZ);
    if (!IsGeodeticValid(latitude, longitude, airH)) {
        fprintf(stderr, "air geodetic coordinates is not valid\n");
        return (CartesianInterpolator){0, 0, 0, 0, 0, 0, 0, 0};
    }
    CartesianInterpolator interpolator = {
        .groundX = groundX,
        .groundY = groundY,
        .groundZ = groundZ,
        .groundH = groundH,
        .airX = airX,
        .airY = airY,
        .airZ = airZ,
        .airH = airH
    };
    return interpolator;
}

void calcCartesian(const CartesianInterpolator *interpolator, double *x, double *y, double *z){
    const double rate = (interpolator->airH - interpolator->groundH) / (interpolator->groundZ - interpolator->groundH);
    *x = interpolator->groundX + (interpolator->airX - interpolator->groundX) * rate;
    *y = interpolator->groundY + (interpolator->airY - interpolator->groundY) * rate;
    *z = interpolator->groundZ + (interpolator->airZ - interpolator->groundZ) * rate;
}