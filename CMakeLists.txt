cmake_minimum_required(VERSION 3.21)
project(FY3G_Resampling C)

find_package(HDF5 REQUIRED COMPONENTS C)

include_directories(${HDF5_INCLUDE_DIRS})
include_directories(./include)

set(SOURCE_FILES
    src/interface.c
    src/networking.c
    src/geotransfer.c
)

add_library(FY3G_Resampling SHARED
    ${SOURCE_FILES}
)

target_include_directories(FY3G_Resampling PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(FY3G_Resampling_exe main.c)
target_link_libraries(FY3G_Resampling_exe FY3G_Resampling ${HDF5_LIBRARIES})