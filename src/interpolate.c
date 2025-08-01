#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "data.h"
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

bool GetGeodeticRange(GridInfo** const infoArray, const int lineCount, float *maxLatitude, float *minLatitude, float *maxLongitude, float *minLongitude){
    *maxLatitude = -90, *minLatitude = 90, *maxLongitude = -180, *minLongitude = 180;
    const float firstLongitude = infoArray[0][0].groundL; // to check if the longitude will over 180 after wrap
    for (int lineIndex = 0; lineIndex < lineCount; lineIndex++){
        for (int angleIndex = 0; angleIndex < SCAN_ANGLE_COUNT; angleIndex++){
            const float latitude = infoArray[lineIndex][angleIndex].groundB;
            float longitude = infoArray[lineIndex][angleIndex].groundL;
            if (longitude < firstLongitude) longitude += 360; // wrap to 0-360 to keep it monotonic
            if (latitude > *maxLatitude) *maxLatitude = latitude;
            if (latitude < *minLatitude) *minLatitude = latitude;
            if (longitude > *maxLongitude) *maxLongitude = longitude;
            if (longitude < *minLongitude) *minLongitude = longitude;
        }
    }
    return true;
}

unsigned int SearchLineIndex(const float latitude, unsigned int bias,GridInfo** const infoArray, unsigned int left, unsigned int right){
    if (left == right) return left;
    const unsigned int mid = left + (right - left) / 2;
    const float midLatitude = infoArray[mid][bias].groundB;
    if (midLatitude < latitude) return SearchLineIndex(latitude, bias, infoArray, mid + 1, right);
    else return SearchLineIndex(latitude, bias, infoArray, left, mid);
}

float QueryClipMaxLongitude(const float minClipLatitude, const float maxClipLatitude, GridInfo** const infoArray, const unsigned int lineCount){
    /**
     * @brief Query the maximum longitude of the clip
     * @param minClipLatitude: the minimum latitude of the clip
     * @param maxClipLatitude: the maximum latitude of the clip
     * @param infoArray: the info array
     * @param lineCount: the line count
     * @return the maximum longitude of the clip
     */
    float maxLongitude = -180;
    const float centerClipLatitude = (minClipLatitude + maxClipLatitude) / 2;
    unsigned int leftLineIndex = fmin(SearchLineIndex(centerClipLatitude, 0, infoArray, 0, lineCount), SearchLineIndex(centerClipLatitude, SCAN_ANGLE_COUNT - 1, infoArray, 0, lineCount));
    unsigned int rightLineIndex = fmax(SearchLineIndex(maxClipLatitude, 0, infoArray, 0, lineCount), SearchLineIndex(maxClipLatitude, SCAN_ANGLE_COUNT - 1, infoArray, 0, lineCount));
    if (rightLineIndex == lineCount) rightLineIndex--;
    for (unsigned int lineIndex = leftLineIndex; lineIndex <= rightLineIndex; lineIndex++){
        for (unsigned int angleIndex = 0; angleIndex < SCAN_ANGLE_COUNT; angleIndex++){
            if (infoArray[lineIndex][angleIndex].groundB < minClipLatitude || infoArray[lineIndex][angleIndex].groundB > maxClipLatitude)
                continue;
            maxLongitude = fmax(maxLongitude, infoArray[lineIndex][angleIndex].groundL);
        }
    }
    return maxLongitude;
}
bool InitClipGridArray(const HDFDataset* dataset, int gridSize, int initHeight, const int heightGap, const int heightCount, ClipGridResult* finalGrid){
    /**
     * @brief Initialize the (final) clip grid array
     * @param dataset: the dataset
     * @param gridSize: the flat grid size(m)
     * @param initHeight: the initial height(m)
     * @param heightGap: the height gap(m)
     * @param heightCount: the height count
     * @param finalGrid: the final grid to store the data
     * @return true if successful, false otherwise
     */
    const unsigned int lineCount = dataset->globalAttribute.scanLineCount;
    const float latitudeGap = (float)gridSize * 180.0f / (M_PI * WGS84_B);
    for (int bandIndex = 0; bandIndex < 2; bandIndex++){
        float globalMaxLatitude, globalMinLatitude, globalMaxLongitude, globalMinLongitude; // longitude is wrapped
        GetGeodeticRange(dataset->infoArray[bandIndex], lineCount, &globalMaxLatitude, &globalMinLatitude, &globalMaxLongitude, &globalMinLongitude);
        finalGrid->clipCount = ceil((globalMaxLatitude - globalMinLatitude) / MAX_LONGITUDE_WIDTH);
        finalGrid->clipGrids[bandIndex] = (ClipGrid*)malloc(finalGrid->clipCount * sizeof(ClipGrid));
        const float realClipLatitudeGap = (globalMaxLatitude - globalMinLatitude) / finalGrid->clipCount;
        const float realClipLatitudeCount = ceil(realClipLatitudeGap / latitudeGap);
        float minClipLongitude = globalMinLongitude;
        for (unsigned int clipIndex = 0; clipIndex < finalGrid->clipCount; clipIndex++){
            ClipGrid* clipGrid = &finalGrid->clipGrids[bandIndex][clipIndex];
            clipGrid->minHeight = initHeight;
            clipGrid->heightGap = heightGap;
            clipGrid->heightCount = heightCount;
            clipGrid->minLatitude = globalMinLatitude + clipIndex * realClipLatitudeGap;
            clipGrid->maxLatitude = clipGrid->minLatitude + realClipLatitudeGap;
            clipGrid->latitudeGap = realClipLatitudeGap;
            clipGrid->latitudeCount = realClipLatitudeCount;
            clipGrid->minLongitude = minClipLongitude;
            const float centerClipLatitude = (clipGrid->minLatitude + clipGrid->maxLatitude) / 2;
            clipGrid->longitudeGap = (float)gridSize * 180.0f / (M_PI * WGS84_A * cos(ToRadians(centerClipLatitude)));
            clipGrid->maxLongitude = QueryClipMaxLongitude(clipGrid->minLatitude, clipGrid->maxLatitude, dataset->infoArray[bandIndex], lineCount);
            clipGrid->longitudeCount = ceil((clipGrid->maxLongitude - clipGrid->minLongitude) / clipGrid->longitudeGap);
            clipGrid->value = (float*)malloc(clipGrid->latitudeCount * clipGrid->longitudeCount * clipGrid->heightCount * sizeof(float));
            minClipLongitude = clipGrid->maxLongitude;
        }
    }
    return true;
}