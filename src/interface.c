#include "interface.h"
#include <H5Ipublic.h>
#include <H5Tpublic.h>
#include <H5public.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>

GridInfo* CreateGridInfo(unsigned int lineIndex, unsigned int angleIndex) {
    /**
    @brief Create GridInfo
    @param lineIndex: the line index
    @param angleIndex: the angle index
    @return the GridInfo pointer, NULL if failed
    */
    GridInfo* info = (GridInfo*)malloc(sizeof(GridInfo));
    if (info != NULL) {
        info->lineIndex = lineIndex;
        info->angleIndex = angleIndex;
        info->heightArray = (float*)malloc(SCAN_HEIGHT_COUNT * sizeof(float));
        info->measuredArray = (float*)malloc(SCAN_HEIGHT_COUNT * sizeof(float));
        if (!info->heightArray || !info->measuredArray) {
            DestroyGridInfo(info); 
            return NULL;
        }
    }
    return info;
}

void DestroyGridInfo(GridInfo* info) {
    /**
    @brief Destroy GridInfo
    @param info: the GridInfo to destroy
    */
    if (info) {
        free(info->heightArray);
        free(info->measuredArray);
        free(info);
    }
}

const char* ConstructPath(const char* pathNames[], const int pathLength){
    /**
    @brief Construct path
    @param pathNames: the names of the path
    @param pathLength: the length of the path
    @return the path
    */
    char* path = (char*)malloc(255 * sizeof(char));
    if (path == NULL){
        fprintf(stderr, "Failed to allocate memory for path\n");
        return NULL;
    }
    for (int i = 0; i < pathLength; i++){
        strcat(path, "/");
        strcat(path, pathNames[i]);
    }
    return path;
}

bool ReadHDF5(const char* filename){
    /**
    @brief Read HDF5 file
    @param filename: the name of the HDF5 file
    @return true if successful, false otherwise
    */
    const char* bandNames[] = {"Ka","Ku"};
    hid_t fileID = H5Fopen(filename, H5F_ACC_RDONLY, H5P_DEFAULT);
    if (fileID < 0){
        fprintf(stderr, "Failed to open file: %s\n", filename);
        return false;
    }
    const char* geolocationPath = ConstructPath((const char*[]){GEOLOCATION_GROUP_NAME}, 1);
    hid_t geolocationID = H5Gopen(fileID, geolocationPath, H5P_DEFAULT);
    if (geolocationID < 0){
        fprintf(stderr, "Failed to open dataset: %s\n", GEOLOCATION_GROUP_NAME);
        return false;
    }
    const char* prePath = ConstructPath((const char*[]){PRE_GROUP_NAME}, 1);
    hid_t preID = H5Gopen(fileID, prePath, H5P_DEFAULT);
    if (preID < 0){
        fprintf(stderr, "Failed to open dataset: %s\n", PRE_GROUP_NAME);
        return false;
    }
    HDFGlobalAttribute globalAttribute;
    if (!ReadGlobalAttribute(fileID, &globalAttribute)){
        fprintf(stderr, "Failed to read global attribute\n");
        return false;
    }

    for (int bandIndex = 0; bandIndex < 2; bandIndex++){
        const char* bandName = bandNames[bandIndex];
        if (!ReadBand(fileID, bandName, &globalAttribute)){
            fprintf(stderr, "Failed to read band: %s\n", bandName);
            return false;
        }
    }

    H5Gclose(geolocationID);
    H5Gclose(preID);
    H5Fclose(fileID);
    return true;
}

bool ReadSingleAttribute(hid_t fileID, const char* attributeName, hid_t typeID, void* buffer){
    /**
    @brief Read single attribute
    @param fileID: the file ID
    @param attributeName: the name of the attribute
    @param typeID: the type of the attribute
    @return true if successful, false otherwise
    @note if typeID is 0, the type of the attribute will be determined by H5Aget_type
    */
    const bool toCreateType = !typeID;
    hid_t attributeID = H5Aopen(fileID, attributeName, H5P_DEFAULT);
    if (attributeID < 0){
        fprintf(stderr, "Failed to open attribute: %s\n", attributeName);
        H5Aclose(attributeID);
        return false;
    }
    if (toCreateType)
        typeID = H5Aget_type(attributeID);
    herr_t status = H5Aread(attributeID, typeID, buffer);
    if (status < 0){
        fprintf(stderr, "Failed to read attribute: %s\n", attributeName);
        H5Aclose(attributeID);
        if (toCreateType)
            H5Tclose(typeID);
        return false;
    }
    if (toCreateType)
        H5Tclose(typeID);
    H5Aclose(attributeID);
    return true;
}

int getNumber(const char* str, int length){
    char temp[5];
    memset(temp, 0, 5);
    strncpy(temp, str, length);
    return atoi(temp);
}

DateTime CreateDateTime(const char* date, const char* time){
    DateTime dateTime;
    dateTime.year = getNumber(date, 4);
    dateTime.month = getNumber(date + 5, 2);
    dateTime.day = getNumber(date + 8, 2);
    dateTime.hour = getNumber(time, 2);
    dateTime.minute = getNumber(time + 3, 2);
    dateTime.second = getNumber(time + 6, 2);
    return dateTime;
}

bool ReadGlobalAttribute(hid_t fileID, HDFGlobalAttribute* globalAttribute){
    /**
    @brief Read global attribute
    @param fileID: the file ID
    @param globalAttribute: the global attribute
    @return true if successful, false otherwise
    */
    const int datestrLength = 15;
    char* startDate = (char*)malloc(datestrLength * sizeof(char));
    char* startTime = (char*)malloc(datestrLength * sizeof(char));
    char* endDate = (char*)malloc(datestrLength * sizeof(char));
    char* endTime = (char*)malloc(datestrLength * sizeof(char));

    if (!ReadSingleAttribute(fileID, "Scan_lines", H5T_NATIVE_INT, &globalAttribute->scanLineCount)){
        fprintf(stderr, "Failed to read scan line count\n");
        return false;
    }
    if (!ReadSingleAttribute(fileID, "Observing Beginning Date", 0, startDate)){
        fprintf(stderr, "Failed to read start date\n");
        return false;
    }
    if (!ReadSingleAttribute(fileID, "Observing Beginning Time", 0, startTime)){
        fprintf(stderr, "Failed to read start time\n");
        return false;
    }
    if (!ReadSingleAttribute(fileID, "Observing Ending Date", 0, endDate)){
        fprintf(stderr, "Failed to read end date\n");
        return false;
    }
    if (!ReadSingleAttribute(fileID, "Observing Ending Time", 0, endTime)){
        fprintf(stderr, "Failed to read end time\n");
        return false;
    }
    
    globalAttribute->startDateTime = CreateDateTime(startDate, startTime);
    globalAttribute->endDateTime = CreateDateTime(endDate, endTime);
    free(startDate);
    free(startTime);
    free(endDate);
    free(endTime);
    return true;
}

hid_t GetDatasetID(hid_t fileID, const char* path){
    hid_t datasetID = H5Dopen(fileID, path, H5P_DEFAULT);
    if (datasetID < 0)
        fprintf(stderr, "Failed to open dataset: %s\n", path);
    return datasetID;
}

bool GetRequiredDatasetID(hid_t fileID, const char* bandName, HDFBandRequired* required){
    required->elevationID = GetDatasetID(fileID, ConstructPath((const char*[]){GEOLOCATION_GROUP_NAME, bandName, "elevation"}, 3));
    if (required->elevationID < 0){
        fprintf(stderr, "Failed to open dataset: %s\n", "elevation");
        return false;
    }
    required->latitudeID = GetDatasetID(fileID, ConstructPath((const char*[]){GEOLOCATION_GROUP_NAME, bandName, "Latitude"}, 3));
    if (required->latitudeID < 0){
        fprintf(stderr, "Failed to open dataset: %s\n", "Latitude");
        return false;
    }
    required->longitudeID = GetDatasetID(fileID, ConstructPath((const char*[]){GEOLOCATION_GROUP_NAME, bandName, "Longitude"}, 3));
    if (required->longitudeID < 0){
        fprintf(stderr, "Failed to open dataset: %s\n", "Longitude");
        return false;
    }
    required->groundHeightID = GetDatasetID(fileID, ConstructPath((const char*[]){GEOLOCATION_GROUP_NAME, bandName, "ellipsoidBinOffset"}, 3));
    if (required->groundHeightID < 0){
        fprintf(stderr, "Failed to open dataset: %s\n", "ellipsoidBinOffset");
        return false;
    }
    required->zenithID = GetDatasetID(fileID, ConstructPath((const char*[]){GEOLOCATION_GROUP_NAME, bandName, "localZenithAngle"}, 3));
    if (required->zenithID < 0){
        fprintf(stderr, "Failed to open dataset: %s\n", "localZenithAngle");
        return false;
    }
    required->valueID = GetDatasetID(fileID, ConstructPath((const char*[]){PRE_GROUP_NAME, bandName, "zFactorMeasured"}, 3));
    if (required->valueID < 0){
        fprintf(stderr, "Failed to open dataset: %s\n", "zFactorMeasured");
        return false;
    }
    required->binClutterID = GetDatasetID(fileID, ConstructPath((const char*[]){PRE_GROUP_NAME, bandName, "binClutterFreeBottom"}, 3));
    if (required->binClutterID < 0){
        fprintf(stderr, "Failed to open dataset: %s\n", "binClutterFreeBottom");
        return false;
    }
    return true;
}

bool ReadSingleDataset(int rank, hid_t datasetID, hsize_t* offset, hsize_t* count, void* buffer){
    /**
    @brief Read single dataset
    @param datasetID: the dataset ID
    @param offset: the offset
    @param count: the count
    @param buffer: the buffer
    @return true if successful, false otherwise
    */
    hid_t dataspaceID = H5Dget_space(datasetID);
    if (dataspaceID < 0){
        fprintf(stderr, "Failed to get dataspace\n");
        H5Sclose(dataspaceID);
        return false;
    }
    herr_t status = H5Sselect_hyperslab(dataspaceID, H5S_SELECT_SET, offset, NULL, count, NULL);
    if (status < 0){
        fprintf(stderr, "Failed to select hyperslab\n");
        H5Sclose(dataspaceID);
        return false;
    }

    hid_t memspaceID = H5Screate_simple(rank, count, NULL);
    status = H5Dread(datasetID, H5T_NATIVE_FLOAT, memspaceID, dataspaceID, H5P_DEFAULT, buffer);
    if (status < 0){
        fprintf(stderr, "Failed to read data\n");
        H5Sclose(dataspaceID);
        H5Sclose(memspaceID);
        return false;
    }
    H5Sclose(dataspaceID);
    H5Sclose(memspaceID);
    return true;
}

bool ReadSingleScanLine(int lineIndex, const HDFBandRequired* required, GridInfo* infoLine){
    /**
    @brief Read single scan line
    @param lineIndex: the line index
    @param required: the required dataset ID
    @param infoLine: the info line to store the data
    @return true if successful, false otherwise
    */
    bool success = true;
    float elevation[SCAN_ANGLE_COUNT];
    float latitude[SCAN_ANGLE_COUNT][2];
    float longitude[SCAN_ANGLE_COUNT][2];
    float groundHeight[SCAN_ANGLE_COUNT];
    float zenith[SCAN_ANGLE_COUNT];
    float value[SCAN_ANGLE_COUNT][SCAN_HEIGHT_COUNT];
    float binClutter[SCAN_ANGLE_COUNT];
    hsize_t offset2D[2] = {lineIndex, 0};
    hsize_t offset3D[3] = {lineIndex, 0, 0};
    hsize_t count2D[2] = {1, SCAN_ANGLE_COUNT};
    hsize_t count3Dint[3] = {1, SCAN_ANGLE_COUNT, 2};
    hsize_t count3Dvalue[3] = {1, SCAN_ANGLE_COUNT, SCAN_HEIGHT_COUNT};
    if (!ReadSingleDataset(2, required->elevationID, offset2D, count2D, elevation)){
        fprintf(stderr, "Failed to read elevation\n");
        success = false;
    }
    if (!ReadSingleDataset(3, required->latitudeID, offset3D, count3Dint, latitude)){
        fprintf(stderr, "Failed to read latitude\n");
        success = false;
    }
    if (!ReadSingleDataset(3, required->longitudeID, offset3D, count3Dint, longitude)){
        fprintf(stderr, "Failed to read longitude\n");
        success = false;
    }
    if (!ReadSingleDataset(2, required->groundHeightID, offset2D, count2D, groundHeight)){
        fprintf(stderr, "Failed to read ground height\n");
        success = false;
    }
    if (!ReadSingleDataset(2, required->zenithID, offset2D, count2D, zenith)){
        fprintf(stderr, "Failed to read zenith\n");
        success = false;
    }
    if (!ReadSingleDataset(3, required->valueID, offset3D, count3Dvalue, value)){
        fprintf(stderr, "Failed to read value\n");
        success = false;
    }
    if (!ReadSingleDataset(2, required->binClutterID, offset2D, count2D, binClutter)){
        fprintf(stderr, "Failed to read bin clutter\n");
        success = false;
    }
    if (success){
        for (int angleIndex = 0; angleIndex < SCAN_ANGLE_COUNT; angleIndex++){
            infoLine[angleIndex].lineIndex = lineIndex;
            infoLine[angleIndex].angleIndex = angleIndex;
            infoLine[angleIndex].groundL = latitude[angleIndex][0];
            infoLine[angleIndex].groundB = longitude[angleIndex][0];
            infoLine[angleIndex].groundH = groundHeight[angleIndex];
            infoLine[angleIndex].airL = latitude[angleIndex][1];
            infoLine[angleIndex].airB = longitude[angleIndex][1];
            infoLine[angleIndex].zeta = zenith[angleIndex];
            infoLine[angleIndex].evaluation = elevation[angleIndex];
            infoLine[angleIndex].measuredArray = value[angleIndex];
            infoLine[angleIndex].heightArray = value[angleIndex];
            infoLine[angleIndex].clutterFreeBottomIndex = binClutter[angleIndex];
        }
    }
    return success;
}

bool ReadBand(hid_t fileID, const char* bandName, HDFGlobalAttribute* globalAttribute){
    /**
    @brief Read band
    @param fileID: the file ID
    @param bandName: the name of the band
    @param globalAttribute: the global attribute
    @return true if successful, false otherwise
    */
    HDFBandRequired required;
    if (!GetRequiredDatasetID(fileID, bandName, &required)){
        fprintf(stderr, "Failed to get all required dataset ID\n");
        return false;
    }
    GridInfo **infoArray = (GridInfo**)malloc(globalAttribute->scanLineCount * sizeof(GridInfo*));

    #pragma omp parallel for
    {
    for (int lineIndex = 0; lineIndex < globalAttribute->scanLineCount; lineIndex++){
        GridInfo* infoLine = (GridInfo*)malloc(SCAN_ANGLE_COUNT * sizeof(GridInfo));
        ReadSingleScanLine(lineIndex, &required, infoLine);
        infoArray[lineIndex] = infoLine;
    }
    }

    // free all resources
    H5Dclose(required.elevationID);
    H5Dclose(required.latitudeID);
    H5Dclose(required.longitudeID);
    H5Dclose(required.zenithID);
    H5Dclose(required.heightID);
    H5Dclose(required.groundHeightID);
    H5Dclose(required.valueID);
    H5Dclose(required.binClutterID);

    for (int lineIndex = 0; lineIndex < globalAttribute->scanLineCount; lineIndex++){
        for (int angleIndex = 0; angleIndex < SCAN_ANGLE_COUNT; angleIndex++)
            DestroyGridInfo(&infoArray[lineIndex][angleIndex]);
        DestroyGridInfo(infoArray[lineIndex]);
    }
    free(infoArray);
    return true;
}
