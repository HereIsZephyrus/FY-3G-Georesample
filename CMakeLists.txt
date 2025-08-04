cmake_minimum_required(VERSION 3.21)
include(ExternalProject)
project(FY3G_Resampling C)

set(HDF5_INSTALL_DIR ${CMAKE_BINARY_DIR}/hdf5_install)
ExternalProject_Add(
    hdf5
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/hdf5
    BINARY_DIR ${CMAKE_BINARY_DIR}/hdf5_build
    INSTALL_DIR ${HDF5_INSTALL_DIR}
    CMAKE_ARGS
        -DCMAKE_BUILD_TYPE=Release
        -DCMAKE_INSTALL_PREFIX=${HDF5_INSTALL_DIR}
        -DHDF5_ENABLE_THREADSAFE=ON
        -DHDF5_BUILD_HL_LIB=OFF
        -DHDF5_ENABLE_ALL_WARNINGS=ON
        -DHDF5_BUILD_TOOLS=OFF
        -DHDF5_ENABLE_SZIP_SUPPORT=OFF
        -DHDF5_ENABLE_ZLIB_SUPPORT=ON
)
set(CMAKE_C_STANDARD 11)
set(HDF5_INCLUDE_DIR ${HDF5_INSTALL_DIR}/include)
set(HDF5_LIB_DIR ${HDF5_INSTALL_DIR}/lib)
link_directories(${HDF5_LIB_DIR})

find_package(OpenMP REQUIRED)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fopenmp")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -O0 -Wall -Wextra")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -Wall -march=native")

add_subdirectory(libspatialindex)
add_subdirectory(tests/Unity)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/libspatialindex/include)
include_directories(${HDF5_INCLUDE_DIRS})
include_directories(./include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/tests/Unity/src)
include_directories(${OpenMP_INCLUDE_DIRS})

set(TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tests)
set(TEST_FILES
    ${TEST_DIR}/Unity/src/unity.c
    ${TEST_DIR}/unit_geotransfer.c
    ${TEST_DIR}/unit_readHDF.c
    ${TEST_DIR}/unit_interpolate.c
    ${TEST_DIR}/unit_RTree.c
    ${TEST_DIR}/unit_KDTree.c
    ${TEST_DIR}/unit_AVLTree.c
    ${TEST_DIR}/test_suites.c
)

set(SOURCE_FILES
    src/rstartree.c
    src/avltree.c
    src/kdtree.c
    src/interface.c
    src/interpolate.c
    src/geotransfer.c
    src/data.c
    src/core.c
    src/index.c
)

add_library(FY3G_Resampling SHARED
    ${SOURCE_FILES}
)
add_dependencies(FY3G_Resampling hdf5)

target_include_directories(FY3G_Resampling PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(FY3G_Resampling_exe main.c)
add_dependencies(FY3G_Resampling_exe hdf5)
target_link_libraries(FY3G_Resampling_exe
    FY3G_Resampling
    m
    libhdf5.so
    OpenMP::OpenMP_C
    spatialindex
    spatialindex_c
)

add_executable(FY3G_Resampling_test ${TEST_FILES})
add_dependencies(FY3G_Resampling_test hdf5)
target_include_directories(FY3G_Resampling_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/Unity/src
)
target_compile_definitions(FY3G_Resampling_test PRIVATE UNITY_INCLUDE_DOUBLE)
target_link_libraries(FY3G_Resampling_test
    FY3G_Resampling
    m
    libhdf5.so
    OpenMP::OpenMP_C
    spatialindex
    spatialindex_c
)