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
    char orbitDirection;

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
    if (!ReadSingleAttribute(fileID, "Orbit Direction", 0, &orbitDirection)){
        fprintf(stderr, "Failed to read orbit direction\n");
        return false;
    }
    
    globalAttribute->startDateTime = CreateDateTime(startDate, startTime);
    globalAttribute->endDateTime = CreateDateTime(endDate, endTime);
    globalAttribute->ascending = orbitDirection == 'A';
    free(startDate);
    free(startTime);
    free(endDate);
    free(endTime);
    return true;
}

hid_t GetDatasetID(hid_t fileID, const char* path){
    /**
    @brief Get dataset ID
    @param fileID: the file ID
    @param path: the path of the dataset
    @return the dataset ID
    */
    hid_t datasetID = H5Dopen(fileID, path, H5P_DEFAULT);
    if (datasetID < 0)
        fprintf(stderr, "Failed to open dataset: %s\n", path);
    return datasetID;
}

bool GetRequiredDatasetID(hid_t fileID, const char* bandName, HDFBandRequired* required){
    /**
    @brief Get required dataset ID
    @param fileID: the file ID
    @param bandName: the name of the band
    @param required: the required dataset ID
    @return true if successful, false otherwise
    */
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
            infoLine[angleIndex].groundL = longitude[angleIndex][0];
            infoLine[angleIndex].groundB = latitude[angleIndex][0];
            infoLine[angleIndex].groundH = groundHeight[angleIndex];
            infoLine[angleIndex].airL = longitude[angleIndex][1];
            infoLine[angleIndex].airB = latitude[angleIndex][1];
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
    
    const hsize_t totalLines = globalAttribute->scanLineCount;
    const hsize_t numBatches = (totalLines + BATCH_SIZE - 1) / BATCH_SIZE;
    const hsize_t restBatchSize = totalLines % BATCH_SIZE;
    
    bool success = true;
    BatchReadContext ctxfull, ctxrest;
    if (!InitBatchReadContext(&ctxfull, BATCH_SIZE) || !InitBatchReadContext(&ctxrest, restBatchSize))
        success = false;
    else {
        #pragma omp for schedule(dynamic)
        for (hsize_t batchIdx = 0; batchIdx < numBatches; batchIdx++) {
            hsize_t startLine = batchIdx * BATCH_SIZE;
            if (startLine + BATCH_SIZE > totalLines){
                if (!ReadBatchScanLines(startLine, totalLines - startLine, &required, &ctxrest, infoArray)) {
                    fprintf(stderr, "Failed to read batch starting at line %lu\n", startLine);
                    success = false;
                }
            }
            else {
                if (!ReadBatchScanLines(startLine, BATCH_SIZE, &required, &ctxfull, infoArray)) {
                    fprintf(stderr, "Failed to read batch starting at line %lu\n", startLine);
                    success = false;
                }
            }
        }
        DestroyBatchReadContext(&ctxfull);
        DestroyBatchReadContext(&ctxrest);
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
    
    return success;
}

bool WriteHDF5(const char* filename, const GeodeticGrid* dataset, const HDFGlobalAttribute* globalAttribute){
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

    //write data
    hsize_t dims[3] = {dataset->lineCount, SCAN_ANGLE_COUNT, dataset->heightCount};
    for (int bandIndex = 0; bandIndex < 2; bandIndex++){
        bool success = true;
        const char* bandName = BAND_NAMES[bandIndex];
        hid_t bandGroupID = H5Gcreate(fileID, ConstructPath((const char*[]){bandName}, 1), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        if (bandGroupID < 0){
            fprintf(stderr, "Failed to create group: %s\n", bandName);
            H5Gclose(bandGroupID);
            H5Fclose(fileID);
            return false;
        }
        hid_t dataspaceID = H5Screate_simple(3, dims, NULL);
        if (dataspaceID < 0){
            fprintf(stderr, "Failed to create dataspace: %s\n", bandName);
            H5Sclose(dataspaceID);
            H5Gclose(bandGroupID);
            H5Fclose(fileID);
            return false;
        }
        hid_t latitudeID = H5Dcreate(bandGroupID, "Latitude", H5T_NATIVE_FLOAT, dataspaceID, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        hid_t longitudeID = H5Dcreate(bandGroupID, "Longitude", H5T_NATIVE_FLOAT, dataspaceID, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        hid_t elevationID = H5Dcreate(bandGroupID, "Elevation", H5T_NATIVE_FLOAT, dataspaceID, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        hid_t valueID = H5Dcreate(bandGroupID, "Value", H5T_NATIVE_FLOAT, dataspaceID, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        if (latitudeID < 0 || longitudeID < 0 || elevationID < 0 || valueID < 0){
            fprintf(stderr, "Failed to create dataset: %s\n", bandName);
            success = false;
        }
        herr_t status = H5Dwrite(latitudeID, H5T_IEEE_F32LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, dataset->latitudeArray[bandIndex]);
        if (status < 0){
            fprintf(stderr, "Failed to write latitude\n");
            success = false;
        }
        status = H5Dwrite(longitudeID, H5T_IEEE_F32LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, dataset->longitudeArray[bandIndex]);
        if (status < 0){
            fprintf(stderr, "Failed to write longitude\n");
            success = false;
        }
        status = H5Dwrite(elevationID, H5T_IEEE_F32LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, dataset->elevationArray[bandIndex]);
        if (status < 0){
            fprintf(stderr, "Failed to write elevation\n");
            success = false;
        }
        status = H5Dwrite(valueID, H5T_IEEE_F32LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, dataset->valueArray[bandIndex]);
        if (status < 0){
            fprintf(stderr, "Failed to write value\n");
            success = false;
        }
        H5Sclose(dataspaceID);
        H5Dclose(latitudeID);
        H5Dclose(longitudeID);
        H5Dclose(elevationID);
        H5Dclose(valueID);
        H5Gclose(bandGroupID);
        if (!success)
            return false;
    }

    //write global attribute
    const char* orbitDirection = globalAttribute->ascending ? "A" : "D";
    hid_t attrDataspaceID = H5Screate(H5S_SCALAR);
    if (attrDataspaceID < 0){
        fprintf(stderr, "Failed to create dataspace: %s\n", "Scan_Lines");
        H5Fclose(fileID);
        return false;
    }
    hid_t scanLineCountID = H5Acreate(fileID, "Scan_Lines", H5T_NATIVE_INT, attrDataspaceID, H5P_DEFAULT, H5P_DEFAULT);
    hid_t strType = H5Tcopy(H5T_C_S1);
    H5Tset_size(strType, 20);
    if (scanLineCountID < 0)
        fprintf(stderr, "Failed to create dataset: %s\n", "scanLineCount");
    hid_t startDateTimeID = H5Acreate(fileID, "Observing_Beginning_DateTime", strType, attrDataspaceID, H5P_DEFAULT, H5P_DEFAULT);
    if (startDateTimeID < 0)
        fprintf(stderr, "Failed to create dataset: %s\n", "startDateTime");
    hid_t endDateTimeID = H5Acreate(fileID, "Observing_Ending_DateTime", strType, attrDataspaceID, H5P_DEFAULT, H5P_DEFAULT);
    if (endDateTimeID < 0)
        fprintf(stderr, "Failed to create dataset: %s\n", "endDateTime");
    hid_t orbitDirectionID = H5Acreate(fileID, "Orbit_Direction", H5T_NATIVE_CHAR, attrDataspaceID, H5P_DEFAULT, H5P_DEFAULT);
    if (orbitDirectionID < 0)
        fprintf(stderr, "Failed to create dataset: %s\n", "orbitDirection");
    herr_t status = H5Awrite(scanLineCountID, H5T_NATIVE_INT, &globalAttribute->scanLineCount);
    if (status < 0)
        fprintf(stderr, "Failed to write dataset: %s\n", "scanLineCount");
    status = H5Awrite(startDateTimeID, strType, ConstructDateTimeString(&globalAttribute->startDateTime));
    if (status < 0)
        fprintf(stderr, "Failed to write dataset: %s\n", "startDateTime");
    status = H5Awrite(endDateTimeID, strType, ConstructDateTimeString(&globalAttribute->endDateTime));
    if (status < 0)
        fprintf(stderr, "Failed to write dataset: %s\n", "endDateTime");
    status = H5Awrite(orbitDirectionID, H5T_NATIVE_CHAR, orbitDirection);
    if (status < 0)
        fprintf(stderr, "Failed to write dataset: %s\n", "orbitDirection");
    H5Tclose(strType);
    H5Aclose(scanLineCountID);
    H5Aclose(startDateTimeID);
    H5Aclose(endDateTimeID);
    H5Aclose(orbitDirectionID);
    H5Sclose(attrDataspaceID);
    H5Fclose(fileID);
    return true;
}

bool InitBatchReadContext(BatchReadContext* ctx, hsize_t batchSize) {
    hsize_t dims2D[2] = {batchSize, SCAN_ANGLE_COUNT};
    hsize_t dims3D_2[3] = {batchSize, SCAN_ANGLE_COUNT, 2};
    hsize_t dims3D_500[3] = {batchSize, SCAN_ANGLE_COUNT, SCAN_HEIGHT_COUNT};
    
    ctx->memspace2D = H5Screate_simple(2, dims2D, NULL);
    ctx->memspace3D_2 = H5Screate_simple(3, dims3D_2, NULL);
    ctx->memspace3D_500 = H5Screate_simple(3, dims3D_500, NULL);
    
    ctx->elevation_batch = (float*)malloc(batchSize * SCAN_ANGLE_COUNT * sizeof(float));
    ctx->latitude_batch = (float*)malloc(batchSize * SCAN_ANGLE_COUNT * 2 * sizeof(float));
    ctx->longitude_batch = (float*)malloc(batchSize * SCAN_ANGLE_COUNT * 2 * sizeof(float));
    ctx->groundHeight_batch = (float*)malloc(batchSize * SCAN_ANGLE_COUNT * sizeof(float));
    ctx->zenith_batch = (float*)malloc(batchSize * SCAN_ANGLE_COUNT * sizeof(float));
    ctx->value_batch = (float*)malloc(batchSize * SCAN_ANGLE_COUNT * SCAN_HEIGHT_COUNT * sizeof(float));
    ctx->binClutter_batch = (float*)malloc(batchSize * SCAN_ANGLE_COUNT * sizeof(float));
    ctx->height_batch = (float*)malloc(batchSize * SCAN_ANGLE_COUNT * SCAN_HEIGHT_COUNT * sizeof(float));
    
    return (ctx->elevation_batch && ctx->latitude_batch && ctx->longitude_batch && 
            ctx->groundHeight_batch && ctx->zenith_batch && ctx->value_batch && 
            ctx->binClutter_batch && ctx->height_batch);
}

void DestroyBatchReadContext(BatchReadContext* ctx) {
    H5Sclose(ctx->memspace2D);
    H5Sclose(ctx->memspace3D_2);
    H5Sclose(ctx->memspace3D_500);
    
    free(ctx->elevation_batch);
    free(ctx->latitude_batch);
    free(ctx->longitude_batch);
    free(ctx->groundHeight_batch);
    free(ctx->zenith_batch);
    free(ctx->value_batch);
    free(ctx->binClutter_batch);
    free(ctx->height_batch);
}

bool ReadBatchDataset(hid_t datasetID, int rank, int dim3, hsize_t startLine, hsize_t batchSize, hid_t memspaceID, void* buffer) {
    /**
    @brief Read batch dataset
    @param datasetID: the dataset ID
    @param rank: the rank of the dataset
    @param dim3: the last dimension length of the dataset, if rank == 2, dim3 = 0
    @param startLine: the start line
    @param batchSize: the batch size
    @param memspaceID: the memory space ID
    @param buffer: the buffer to store the data
    @return true if successful, false otherwise
    */
    hid_t dataspaceID = H5Dget_space(datasetID);
    if (dataspaceID < 0) return false;
    
    hsize_t offset[3] = {startLine, 0, 0};
    hsize_t count[3] = {batchSize, SCAN_ANGLE_COUNT, dim3};
    if (dim3 > 0 && rank ==2){
        fprintf(stderr, "dim3 > 0 and rank == 2\n");
        return false;
    }
    
    herr_t status = H5Sselect_hyperslab(dataspaceID, H5S_SELECT_SET, offset, NULL, count, NULL);
    if (status < 0) {
        H5Sclose(dataspaceID);
        return false;
    }
    
    status = H5Dread(datasetID, H5T_NATIVE_FLOAT, memspaceID, dataspaceID, H5P_DEFAULT, buffer);
    H5Sclose(dataspaceID);
    
    return status >= 0;
}

bool ReadBatchScanLines(hsize_t startLine, hsize_t batchSize, const HDFBandRequired* required, BatchReadContext* ctx, GridInfo** infoArray) {
    /**
    @brief Read batch scan lines
    @param startLine: the start line
    @param batchSize: the batch size
    @param required: the required dataset ID
    @param ctx: the context
    @param infoArray: the info array to store the data
    @return true if successful, false otherwise
    */
    if (!ReadBatchDataset(required->elevationID, 2, 0, startLine, batchSize, ctx->memspace2D, ctx->elevation_batch) ||
        !ReadBatchDataset(required->latitudeID, 3, 2, startLine, batchSize, ctx->memspace3D_2, ctx->latitude_batch) ||
        !ReadBatchDataset(required->longitudeID, 3, 2, startLine, batchSize, ctx->memspace3D_2, ctx->longitude_batch) ||
        !ReadBatchDataset(required->groundHeightID, 2, 0, startLine, batchSize, ctx->memspace2D, ctx->groundHeight_batch) ||
        !ReadBatchDataset(required->zenithID, 2, 0, startLine, batchSize, ctx->memspace2D, ctx->zenith_batch) ||
        !ReadBatchDataset(required->valueID, 3, SCAN_HEIGHT_COUNT, startLine, batchSize, ctx->memspace3D_500, ctx->value_batch) ||
        !ReadBatchDataset(required->heightID, 3, SCAN_HEIGHT_COUNT, startLine, batchSize, ctx->memspace3D_500, ctx->height_batch) ||
        !ReadBatchDataset(required->binClutterID, 2, 0, startLine, batchSize, ctx->memspace2D, ctx->binClutter_batch)) {
        return false;
    }
    
    #pragma omp parallel for
    for (hsize_t i = 0; i < batchSize; i++) {
        hsize_t lineIdx = startLine + i;
        GridInfo* infoLine = (GridInfo*)malloc(SCAN_ANGLE_COUNT * sizeof(GridInfo));
        if (!infoLine) continue;
        
        for (int angleIndex = 0; angleIndex < SCAN_ANGLE_COUNT; angleIndex++) {
            size_t base2D = i * SCAN_ANGLE_COUNT + angleIndex;
            size_t base3D_2 = base2D * 2;
            size_t base3D_500 = base2D * SCAN_HEIGHT_COUNT;
            
            infoLine[angleIndex].lineIndex = lineIdx;
            infoLine[angleIndex].angleIndex = angleIndex;
            infoLine[angleIndex].groundL = ctx->longitude_batch[base3D_2];
            infoLine[angleIndex].groundB = ctx->latitude_batch[base3D_2];
            infoLine[angleIndex].groundH = ctx->groundHeight_batch[base2D];
            infoLine[angleIndex].airL = ctx->longitude_batch[base3D_2 + 1];
            infoLine[angleIndex].airB = ctx->latitude_batch[base3D_2 + 1];
            infoLine[angleIndex].zeta = ctx->zenith_batch[base2D];
            infoLine[angleIndex].evaluation = ctx->elevation_batch[base2D];
            infoLine[angleIndex].clutterFreeBottomIndex = ctx->binClutter_batch[base2D];
            infoLine[angleIndex].measuredArray = (float*)malloc(SCAN_HEIGHT_COUNT * sizeof(float));
            infoLine[angleIndex].heightArray = (float*)malloc(SCAN_HEIGHT_COUNT * sizeof(float));
            
            if (infoLine[angleIndex].measuredArray && infoLine[angleIndex].heightArray) {
                memcpy(infoLine[angleIndex].measuredArray, &ctx->value_batch[base3D_500], 
                       SCAN_HEIGHT_COUNT * sizeof(float));
                memcpy(infoLine[angleIndex].heightArray, &ctx->height_batch[base3D_500], 
                       SCAN_HEIGHT_COUNT * sizeof(float));
            }
        }
        infoArray[lineIdx] = infoLine;
    }
    
    return true;
}