#ifndef INTERPOLATE_H
#define INTERPOLATE_H

#include <stddef.h>
#include <stdint.h>
#include <math.h>

typedef struct {
    double groundX, groundY, groundZ, groundH;
    double airX, airY, airZ, airH;
} CartesianInterpolator;

CartesianInterpolator calcInterParams(  const double groundX, const double groundY, const double groundZ, const double groundH,
                                        const double latitude, const double longitude, const double zeta);

void calcCartesian(const CartesianInterpolator *interpolator, double *x, double *y, double *z);
#endif