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
    char* path = (char*)malloc(pathLength * sizeof(char));
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
    hid_t geolocationID = H5Dopen(fileID, geolocationPath, H5P_DEFAULT);
    if (geolocationID < 0){
        fprintf(stderr, "Failed to open dataset: %s\n", GEOLOCATION_GROUP_NAME);
        return false;
    }
    const char* prePath = ConstructPath((const char*[]){PRE_GROUP_NAME}, 1);
    hid_t preID = H5Dopen(fileID, prePath, H5P_DEFAULT);
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

    H5Dclose(geolocationID);
    H5Dclose(preID);
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
    */
    hid_t attributeID = H5Aopen(fileID, attributeName, H5P_DEFAULT);
    if (attributeID < 0){
        fprintf(stderr, "Failed to open attribute: %s\n", attributeName);
        return false;
    }
    herr_t status = H5Aread(attributeID, typeID, buffer);
    if (status < 0){
        fprintf(stderr, "Failed to read attribute: %s\n", attributeName);
        return false;
    }
    H5Aclose(attributeID);
    return true;
}

DateTime CreateDateTime(const char* date, const char* time){
    DateTime dateTime;
    char temp[5];
    strncpy(temp, date, 4);
    dateTime.year = atoi(temp);
    strncpy(temp, date + 5, 2);
    dateTime.month = atoi(temp);
    strncpy(temp, date + 8, 2);
    dateTime.day = atoi(temp);
    strncpy(temp, time, 2);
    dateTime.hour = atoi(temp);
    strncpy(temp, time + 3, 2);
    dateTime.minute = atoi(temp);
    strncpy(temp, time + 6, 2);
    dateTime.second = atoi(temp);
    return dateTime;
}

bool ReadGlobalAttribute(hid_t fileID, HDFGlobalAttribute* globalAttribute){
    /**
    @brief Read global attribute
    @param fileID: the file ID
    @param globalAttribute: the global attribute
    @return true if successful, false otherwise
    */
    char* startDate = (char*)malloc(15 * sizeof(char));
    char* startTime = (char*)malloc(15 * sizeof(char));
    char* endDate = (char*)malloc(15 * sizeof(char));
    char* endTime = (char*)malloc(15 * sizeof(char));

    if (!ReadSingleAttribute(fileID, "Scan_Lines", H5T_NATIVE_INT, &globalAttribute->scanLineCount)){
        fprintf(stderr, "Failed to read scan line count\n");
        return false;
    }
    if (!ReadSingleAttribute(fileID, "Observing_Beginning_Date", H5T_NATIVE_CHAR, startDate)){
        fprintf(stderr, "Failed to read start date\n");
        return false;
    }
    if (!ReadSingleAttribute(fileID, "Observing_Beginning_Time", H5T_NATIVE_CHAR, startTime)){
        fprintf(stderr, "Failed to read start time\n");
        return false;
    }
    if (!ReadSingleAttribute(fileID, "Observing_Ending_Date", H5T_NATIVE_CHAR, endDate)){
        fprintf(stderr, "Failed to read end date\n");
        return false;
    }
    if (!ReadSingleAttribute(fileID, "Observing_Ending_Time", H5T_NATIVE_CHAR, endTime)){
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

bool ReadSingleScanLine(int lineIndex, const HDFBandRequired* required, GridInfo* infoLine){
    /**
    @brief Read single scan line
    @param lineIndex: the line index
    @param required: the required dataset ID
    @param infoLine: the info line to store the data
    @return true if successful, false otherwise
    */
    hsize_t offset2D[2] = {lineIndex, 0};
    hsize_t offset3D[3] = {lineIndex, 0, 0};
    hsize_t count2D[2] = {1, SCAN_ANGLE_COUNT};
    hsize_t count3D[3] = {1, SCAN_ANGLE_COUNT, H5S_UNLIMITED};

    herr_t status = H5Sselect_hyperslab(required->elevationID, H5S_SELECT_SET, offset2D, NULL, count2D, NULL);
    hid_t memspaceID = H5Screate_simple(2, count2D, NULL);
    hid_t dataspaceID = H5Dget_space(required->elevationID);
    float* elevation = (float*)malloc(SCAN_ANGLE_COUNT * sizeof(float));

    /*
    status = H5Dread(required->elevationID, H5T_NATIVE_FLOAT, memspaceID, dataspaceID, H5P_DEFAULT, elevation);
    if (status < 0){
        fprintf(stderr, "Failed to select hyperslab\n");
        return false;
    }

    status = H5Dread(required->elevationID, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, infoLine->heightArray);
    if (status < 0){
        fprintf(stderr, "Failed to read elevation\n");
        return false;
    }

    status = H5Dread(required->latitudeID, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, infoLine->latitudeArray);
    */
    H5Sclose(dataspaceID);
    H5Sclose(memspaceID);
    return true;
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
