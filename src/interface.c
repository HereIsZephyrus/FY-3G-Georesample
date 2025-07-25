#include "interface.h"
#include <H5Ipublic.h>
#include <H5Tpublic.h>
#include <stdlib.h>
#include <string.h>

GridInfo* CreateGridInfo(unsigned int nx, unsigned int ny) {
    /**
    @brief Create GridInfo
    @param nx: the number of columns
    @param ny: the number of rows
    @return the GridInfo pointer, NULL if failed
    */
    GridInfo* info = (GridInfo*)malloc(sizeof(GridInfo));
    if (info != NULL) {
        info->nx = nx;
        info->ny = ny;
        info->heightArray = (double*)malloc(SCAN_HEIGHT_COUNT * sizeof(double));
        info->measuredArray = (double*)malloc(SCAN_HEIGHT_COUNT * sizeof(double));
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
        if (!ReadBand(fileID, geolocationID, preID, bandName)){
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

bool ReadBand(hid_t fileID, hid_t geolocationID, hid_t preID, const char* bandName){
    /**
    @brief Read band
    @param fileID: the file ID
    @param geolocationID: the geolocation ID
    @param preID: the pre ID
    @param bandName: the name of the band
    @return true if successful, false otherwise
    */

    return true;
}
