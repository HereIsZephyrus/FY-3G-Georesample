#ifndef INTERFACE_H
#define INTERFACE_H
#define BATCH_SIZE 500

#include <hdf5.h>
#include "data.h"

typedef struct {
    hid_t elevationID, latitudeID, longitudeID, zenithID, heightID, groundHeightID, valueID, binClutterID;
} HDFBandRequired;

typedef struct {
    hid_t dataspace2D, dataspace3D_2, dataspace3D_500;
    hid_t memspace2D, memspace3D_2, memspace3D_500;
    
    float* elevation_batch;        // [BATCH_SIZE][SCAN_ANGLE_COUNT]
    float* latitude_batch;         // [BATCH_SIZE][SCAN_ANGLE_COUNT][2]
    float* longitude_batch;        // [BATCH_SIZE][SCAN_ANGLE_COUNT][2]
    float* groundHeight_batch;     // [BATCH_SIZE][SCAN_ANGLE_COUNT]
    float* zenith_batch;           // [BATCH_SIZE][SCAN_ANGLE_COUNT]
    float* value_batch;            // [BATCH_SIZE][SCAN_ANGLE_COUNT][SCAN_HEIGHT_COUNT]
    float* binClutter_batch;       // [BATCH_SIZE][SCAN_ANGLE_COUNT]
    float* height_batch;           // [BATCH_SIZE][SCAN_ANGLE_COUNT][SCAN_HEIGHT_COUNT]
} BatchReadContext;

bool InitBatchReadContext(BatchReadContext* ctx, hsize_t batchSize);
void DestroyBatchReadContext(BatchReadContext* ctx);
bool ReadHDF5(const char* filename, HDFDataset* dataset);
bool ReadSingleAttribute(hid_t fileID, const char* attributeName, hid_t typeID, void* buffer);
bool ReadGlobalAttribute(hid_t fileID, HDFGlobalAttribute* globalAttribute);
hid_t GetDatasetID(hid_t fileID, const char* path);
bool GetRequiredDatasetID(hid_t fileID, const char* bandName, HDFBandRequired* required);
bool ReadBand(hid_t fileID, const char* bandName, HDFGlobalAttribute* globalAttribute, GridInfo** infoArray);
bool ReadSingleScanLine(int lineIndex, const HDFBandRequired* required, GridInfo* infoLine);
bool ReadSingleDataset(int rank, hid_t datasetID, hsize_t* offset, hsize_t* count, void* buffer);
char* ConstructPath(const char* pathNames[], const int pathLength);
bool WriteHDF5(const char* filename, const GeodeticGrid* finalGrid, const HDFGlobalAttribute* globalAttribute);
bool ReadBatchScanLines(hsize_t startLine, hsize_t batchSize, const HDFBandRequired* required, BatchReadContext* ctx, GridInfo** infoArray);
bool ReadBatchDataset(hid_t datasetID, int rank, int dim3, hsize_t startLine, hsize_t batchSize, hid_t memspaceID, void* buffer);
#endif