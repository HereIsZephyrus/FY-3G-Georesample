#ifndef CONFIG_H
#define CONFIG_H
#include <stdlib.h>
#define DEFAULT_MAX_LONGITUDE_WIDTH 5 // 5 degrees
#define DEFAULT_K_NEIGHBOR 5
#define DEFAULT_KDTREE_CAPACITY 100000

#define DEFAULT_GRID_SIZE 5000 // 5000m
#define DEFAULT_MINIMAL_HEIGHT 100 // 100m
#define DEFAULT_HEIGHT_GAP 200 // 200m
#define DEFAULT_HEIGHT_COUNT 60

#define DEFAULT_BATCH_SIZE 500 // deprecated

struct Config{
    char input_file_name[256];
    char geo_output_file_name[256];
    char clip_output_file_name[256];
    float max_longitude_width;
    float minimal_height, maximal_height;
    float height_gap;
    unsigned int height_count;
    unsigned int k_neighbor;
    unsigned int kdtree_capacity;
    unsigned int grid_size;
    unsigned int batch_size;
};

extern struct Config *g_config;
#endif
