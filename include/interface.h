#ifndef INTERFACE_H
#define INTERFACE_H
#include <H5Ipublic.h>
#include <hdf5.h>
#include "data.h"
#include "config.h"

typedef struct {
    hid_t elevationID, latitudeID, longitudeID, zenithID, heightID, groundHeightID, valueID, binClutterID;
} HDFBandRequired;

typedef struct {
    hid_t dataspace2D, dataspace3D_2, dataspace3D_500;
    hid_t memspace2D, memspace3D_2, memspace3D_500;
} BatchReadContext;

bool InitBatchReadContext(BatchReadContext* ctx, hsize_t batchSize);
void DestroyBatchReadContext(BatchReadContext* ctx);
bool ReadHDF5(const unsigned int bandIndex, const char* filename, HDFDataset* dataset);
bool ReadSingleAttribute(hid_t fileID, const char* attributeName, hid_t typeID, void* buffer);
bool ReadGlobalAttribute(hid_t fileID, HDFGlobalAttribute* globalAttribute);
hid_t GetDatasetID(hid_t fileID, const char* path);
bool GetRequiredDatasetID(hid_t fileID, const char* bandName, HDFBandRequired* required);
bool ReadBand(hid_t fileID, const char* bandName, HDFGlobalAttribute* globalAttribute, GridInfo** infoArray);
bool ReadSingleScanLine(int lineIndex, const HDFBandRequired* required, GridInfo* infoLine);
bool ReadSingleDataset(int rank, hid_t datasetID, hsize_t* offset, hsize_t* count, void* buffer);
char* ConstructPath(const char* pathNames[], const int pathLength);
bool WriteTotalGeodetic(const unsigned int bandIndex, const char* filename, const GeodeticGrid* finalGrid, const HDFGlobalAttribute* globalAttribute);
bool WriteClipResult(const unsigned int bandIndex, const char* filename, const ClipGridResult* clipResult);
bool WriteGlobalAttribute(hid_t fileID, const HDFGlobalAttribute* globalAttribute);
bool ReadBatchScanLines(hsize_t startLine, hsize_t batchSize, const HDFBandRequired* required, BatchReadContext* ctx, GridInfo** infoArray);
bool ReadBatchDataset(hid_t datasetID, int rank, int dim3, hsize_t startLine, hsize_t batchSize, hid_t memspaceID, void* buffer);
char* ConstructOutputFilename(const char* filename, const char* suffix);
struct Config* ReadConfig(const char* filename);
#endif