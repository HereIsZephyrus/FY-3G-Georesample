#include <stdio.h>
#include <stdlib.h>
#include "interpolate.h"
#include "geotransfer.h"

CartesianInterpolator calcInterParams(  const double groundX, const double groundY, const double groundZ, const double groundH,
                                        const double latitude, const double longitude, const double zeta) {
    /**
     * @brief Calculate the interpolator
     * @param groundX: the ground X coordinate
     * @param groundY: the ground Y coordinate
     * @param groundZ: the ground Z coordinate
     * @param groundH: the ground height
     * @param latitude: the latitude
     * @param longitude: the longitude
     * @param zeta: the zenith angle
     * @return the interpolator
     */
    const double zeta_rad = ToRadians(zeta), latitude_rad = ToRadians(latitude), longitude_rad = ToRadians(longitude);
    const double N = ComputeN(latitude_rad);
    const double alpha = 1 - 1 / (cos(zeta_rad) * cos(zeta_rad));
    const double beta = 2 * groundH / (cos(zeta_rad) * cos(zeta_rad)) +
                        2 * N * (1 - WGS84_E * WGS84_E * sin(latitude_rad) * sin(latitude_rad)) -
                        2 * (cos(latitude_rad) * cos(longitude_rad) * groundX +
                            cos(latitude_rad) * sin(longitude_rad) * groundY +
                            sin(latitude_rad) * groundZ);
    const double gamma = groundX * groundX + groundY * groundY + groundZ * groundZ -
                        groundH * groundH / (cos(zeta_rad) * cos(zeta_rad)) +
                        N * N * (1 - 2 * WGS84_E * WGS84_E * sin(latitude_rad) * sin(latitude_rad) +
                                WGS84_E * WGS84_E * WGS84_E * WGS84_E * sin(latitude_rad) * sin(latitude_rad)) -
                        2 * N * (cos(latitude_rad) * cos(longitude_rad) * groundX +
                                cos(latitude_rad) * sin(longitude_rad) * groundY +
                                groundZ * (1 - WGS84_E * WGS84_E) * sin(latitude_rad));
    const double delta = beta * beta - 4 * alpha * gamma;
    if (delta < 0) {
        fprintf(stderr, "delta is less than 0\n");
        return (CartesianInterpolator){0, 0, 0, 0, 0, 0, 0, 0};
    }
    double airX, airY, airZ, airH = (-beta - sqrt(delta)) / (2 * alpha);
    if (!TransferGeodeticToCartesian(latitude, longitude, airH, &airX, &airY, &airZ)) {
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

Coordinate calcCartesian(const CartesianInterpolator *interpolator, const float queryHeight){
    /**
     * @brief Calculate the Cartesian coordinate
     * @param interpolator: the interpolator
     * @param queryHeight: the query height
     * @return the Cartesian coordinate
     */
    const double rate = (queryHeight - interpolator->groundH) / (interpolator->airH - interpolator->groundH);
    const double x = interpolator->groundX + (interpolator->airX - interpolator->groundX) * rate;
    const double y = interpolator->groundY + (interpolator->airY - interpolator->groundY) * rate;
    const double z = interpolator->groundZ + (interpolator->airZ - interpolator->groundZ) * rate;
    return TransferCartesianToGeodetic(x, y, z, false);
}