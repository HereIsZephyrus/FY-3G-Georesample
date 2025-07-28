#include "interface.h"
#include "data.h"
#include <H5Ipublic.h>
#include <H5Tpublic.h>
#include <H5public.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>

char* ConstructPath(const char* pathNames[], const int pathLength){
    /**
    @brief Construct path
    @param pathNames: the names of the path
    @param pathLength: the length of the path
    @return the path
    */
    if (pathLength == 0)
        return NULL;
    size_t nameLength = 1;
    for (int i = 0; i < pathLength; i++)
        nameLength += strlen(pathNames[i]) + 1;
    char* path = (char*)malloc(nameLength * sizeof(char));
    if (path == NULL){
        fprintf(stderr, "Failed to allocate memory for path\n");
        return NULL;
    }
    path[0] = '\0';
    for (int i = 0; i < pathLength; i++){
        strcat(path, "/");
        strcat(path, pathNames[i]);
    }
    return path;
}

bool ReadHDF5(const char* filename, HDFDataset* dataset){
    /**
    @brief Read HDF5 file
    @param filename: the name of the HDF5 file
    @param dataset: the dataset to store the data
    @return true if successful, false otherwise
    */
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
    if (!ReadGlobalAttribute(fileID, &dataset->globalAttribute)){
        fprintf(stderr, "Failed to read global attribute\n");
        return false;
    }

    for (int bandIndex = 0; bandIndex < 2; bandIndex++){
        const char* bandName = BAND_NAMES[bandIndex];
        dataset->infoArray[bandIndex] = (GridInfo**)malloc(dataset->globalAttribute.scanLineCount * sizeof(GridInfo*));
        if (!ReadBand(fileID, bandName, &dataset->globalAttribute, dataset->infoArray[bandIndex])){
            fprintf(stderr, "Failed to read band: %s\n", bandName);
            return false;
        }
        for (unsigned int lineIndex = DEBUG_INDEX; lineIndex < dataset->globalAttribute.scanLineCount; lineIndex++){
            for (int angleIndex = 0; angleIndex < SCAN_ANGLE_COUNT; angleIndex++)
                DestroyGridInfo(&dataset->infoArray[bandIndex][lineIndex][angleIndex]);
            free(dataset->infoArray[bandIndex][lineIndex]);
        }
        free(dataset->infoArray[bandIndex]);
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
    required->heightID = GetDatasetID(fileID, ConstructPath((const char*[]){GEOLOCATION_GROUP_NAME, bandName, "height"}, 3));
    if (required->heightID < 0){
        fprintf(stderr, "Failed to open dataset: %s\n", "height");
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
    float height[SCAN_ANGLE_COUNT][SCAN_HEIGHT_COUNT];
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
    if (!ReadSingleDataset(3, required->heightID, offset3D, count3Dvalue, height)){
        fprintf(stderr, "Failed to read height\n");
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
            infoLine[angleIndex].clutterFreeBottomIndex = binClutter[angleIndex];
            infoLine[angleIndex].measuredArray = (float*)malloc(SCAN_HEIGHT_COUNT * sizeof(float));
            infoLine[angleIndex].heightArray = (float*)malloc(SCAN_HEIGHT_COUNT * sizeof(float));
            if (!infoLine[angleIndex].measuredArray || !infoLine[angleIndex].heightArray){
                fprintf(stderr, "Failed to allocate memory for measuredArray or heightArray\n");
                success = false;
                break;
            }
            memcpy(infoLine[angleIndex].measuredArray, value[angleIndex], SCAN_HEIGHT_COUNT * sizeof(float));
            memcpy(infoLine[angleIndex].heightArray, height[angleIndex], SCAN_HEIGHT_COUNT * sizeof(float));
        }
    }
    return success;
}

bool ReadBand(hid_t fileID, const char* bandName, HDFGlobalAttribute* globalAttribute, GridInfo** infoArray){
    /**
    @brief Read band
    @param fileID: the file ID
    @param bandName: the name of the band
    @param globalAttribute: the global attribute
    @param infoArray: the info array to store the data
    @return true if successful, false otherwise
    */
    HDFBandRequired required;
    if (!GetRequiredDatasetID(fileID, bandName, &required)){
        fprintf(stderr, "Failed to get all required dataset ID\n");
        return false;
    }
    #pragma omp parallel for shared(infoArray, required, globalAttribute)
    for (unsigned int lineIndex = DEBUG_INDEX; lineIndex < globalAttribute->scanLineCount; lineIndex++){
        GridInfo* infoLine = (GridInfo*)malloc(SCAN_ANGLE_COUNT * sizeof(GridInfo));
        if (!infoLine){
            fprintf(stderr, "Failed to allocate memory for infoLine\n");
            continue;
        }
        if (!ReadSingleScanLine(lineIndex, &required, infoLine)){
            fprintf(stderr, "Failed to read scan line\n");
            free(infoLine);
            continue;
        }
        infoArray[lineIndex] = infoLine;
        printf("process %d from thread %d of %d\n", lineIndex, omp_get_thread_num(), omp_get_num_threads());
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
    return true;
}

bool WriteHDF5(const char* filename, const HDFDataset* dataset){
    /**
    @brief Write HDF5 file
    @param filename: the name of the HDF5 file
    @param dataset: the dataset to write
    @return true if successful, false otherwise
    */
    hid_t fileID = H5Fcreate(filename, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    if (fileID < 0){
        fprintf(stderr, "Failed to create file: %s\n", filename);   
        return false;
    }
    hsize_t dims[3] = {100, SCAN_ANGLE_COUNT, SCAN_HEIGHT_COUNT};
    const hsize_t maxdims[3] = {dataset->globalAttribute.scanLineCount, SCAN_ANGLE_COUNT, SCAN_HEIGHT_COUNT};
    for (int bandIndex = 0; bandIndex < 2; bandIndex++){
        const char* bandName = BAND_NAMES[bandIndex];
        hid_t bandGroupID = H5Gcreate(fileID, ConstructPath((const char*[]){bandName}, 1), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        if (bandGroupID < 0){
            fprintf(stderr, "Failed to create group: %s\n", bandName);
            H5Gclose(bandGroupID);
            H5Fclose(fileID);
            return false;
        }
        hid_t dataspaceID = H5Screate_simple(3, dims, maxdims);
        if (dataspaceID < 0){
            fprintf(stderr, "Failed to create dataspace: %s\n", bandName);
            H5Sclose(dataspaceID);
            H5Gclose(bandGroupID);
            H5Fclose(fileID);
            return false;
        }
        hid_t latitudeID = H5Dcreate(bandGroupID, ConstructPath((const char*[]){bandName, "Latitude"}, 2), H5T_NATIVE_FLOAT, dataspaceID, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        hid_t longitudeID = H5Dcreate(bandGroupID, ConstructPath((const char*[]){bandName, "Longitude"}, 2), H5T_NATIVE_FLOAT, dataspaceID, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        hid_t heightID = H5Dcreate(bandGroupID, "height", H5T_NATIVE_FLOAT, H5S_SCALAR, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        if (latitudeID < 0 || longitudeID < 0 || heightID < 0){
            fprintf(stderr, "Failed to create dataset: %s\n", bandName);
            H5Sclose(dataspaceID);
            H5Dclose(latitudeID);
            H5Dclose(longitudeID);
            H5Dclose(heightID);
            H5Gclose(bandGroupID);
            H5Fclose(fileID);
            return false;
        }
        herr_t status = H5Dwrite(latitudeID, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, dataset->infoArray[bandIndex][0][0].airL);
        H5Sclose(dataspaceID);
        H5Gclose(bandGroupID);
    }
    H5Fclose(fileID);
    return true;
}

bool ConstructFinalGrid(const HDFDataset* dataset, FinalGrid* finalGrid){
    finalGrid->lineCount = dataset->globalAttribute.scanLineCount / 10;
    finalGrid->heightCount = SCAN_HEIGHT_COUNT / 10;
    const int dims[3] = {finalGrid->lineCount, SCAN_ANGLE_COUNT, finalGrid->heightCount};
    const int memSize = dims[0] * dims[1] * dims[2] * sizeof(float);
    for (int bandIndex = 0; bandIndex < 2; bandIndex++){
        finalGrid->latitudeArray[bandIndex] = (float*)malloc(memSize);
        finalGrid->longitudeArray[bandIndex] = (float*)malloc(memSize);
        finalGrid->elevationArray[bandIndex] = (float*)malloc(memSize);
        finalGrid->valueArray[bandIndex] = (float*)malloc(memSize);
        if (!finalGrid->latitudeArray[bandIndex] || !finalGrid->longitudeArray[bandIndex] || !finalGrid->elevationArray[bandIndex] || !finalGrid->valueArray[bandIndex]){
            fprintf(stderr, "Failed to allocate memory for latitudeArray, longitudeArray, elevationArray or valueArray\n");
            return false;
        }
    }
    #pragma omp parallel for shared(dataset, finalGrid)
    for (unsigned int lineIndex = 0; lineIndex < finalGrid->lineCount; lineIndex++)
        for (int bandIndex = 0; bandIndex < 2; bandIndex++)
            for (unsigned int angleIndex = 0; angleIndex < SCAN_ANGLE_COUNT; angleIndex++)
                for (unsigned int heightIndex = 0; heightIndex < finalGrid->heightCount; heightIndex++){
                    const int index = lineIndex * SCAN_ANGLE_COUNT * finalGrid->heightCount + angleIndex * finalGrid->heightCount + heightIndex;
                    finalGrid->latitudeArray[bandIndex][index] = dataset->infoArray[bandIndex][lineIndex][angleIndex].groundB;
                    finalGrid->longitudeArray[bandIndex][index] = dataset->infoArray[bandIndex][lineIndex][angleIndex].groundL;
                    finalGrid->elevationArray[bandIndex][index] = dataset->infoArray[bandIndex][lineIndex][angleIndex].evaluation;
                    finalGrid->valueArray[bandIndex][index] = dataset->infoArray[bandIndex][lineIndex][angleIndex].measuredArray[heightIndex];
                }
    return true;
}