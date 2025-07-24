#include <stdio.h>
#include "geotransfer.h"
#include "interface.h"
#include "networking.h"

bool TransferData(const char *input_file, const char *output_file) {
    /**
     * @brief transfer FY-3G raw HDF5 3D prespirator data to geodetic coordinates
     * @param input_file: path to the raw HDF5 file
     * @param output_file: path to the output file
     * @return true if success, false if failed
    */
    return true;
}

int main(int argc, char *argv[]) {
    /**
     * @brief transfer FY-3G raw HDF5 3D prespirator data to geodetic coordinates
     * @param argv[1]: path to the raw HDF5 file
     * @param argv[2]: path to the output file
     * @return 0 if success, -1 if failed
    */
    if (argc != 3) {
        printf("Usage: %s <input_file> <output_file>\n", argv[0]);
        return -1;
    }
    const char *input_file = argv[1];
    const char *output_file = argv[2];
    return TransferData(input_file, output_file);
}