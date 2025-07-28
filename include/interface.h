#ifndef INTERFACE_H
#define INTERFACE_H
#include <hdf5.h>
#include "data.h"

bool ReadHDF5(const char* filename, HDFDataset* dataset);
bool ReadSingleAttribute(hid_t fileID, const char* attributeName, hid_t typeID, void* buffer);
bool ReadGlobalAttribute(hid_t fileID, HDFGlobalAttribute* globalAttribute);
hid_t GetDatasetID(hid_t fileID, const char* path);
bool GetRequiredDatasetID(hid_t fileID, const char* bandName, HDFBandRequired* required);
bool ReadBand(hid_t fileID, const char* bandName, HDFGlobalAttribute* globalAttribute, GridInfo** infoArray);
bool ReadSingleScanLine(int lineIndex, const HDFBandRequired* required, GridInfo* infoLine);
bool ReadSingleDataset(int rank, hid_t datasetID, hsize_t* offset, hsize_t* count, void* buffer);
char* ConstructPath(const char* pathNames[], const int pathLength);

bool ConstructFinalGrid(const HDFDataset* dataset, FinalGrid* finalGrid);
bool WriteHDF5(const char* filename, const FinalGrid* finalGrid, const HDFGlobalAttribute* globalAttribute);
#endif