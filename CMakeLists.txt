cmake_minimum_required(VERSION 3.21)
project(FY3G_Resampling C)

find_package(HDF5 REQUIRED COMPONENTS C)
find_package(OpenMP REQUIRED)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fopenmp")
add_subdirectory(tests/Unity)

include_directories(${HDF5_INCLUDE_DIRS})
include_directories(./include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/tests/Unity/src)
include_directories(${OpenMP_INCLUDE_DIRS})

set(TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tests)
set(TEST_FILES
    ${TEST_DIR}/Unity/src/unity.c
    ${TEST_DIR}/unit_geotransfer.c
    ${TEST_DIR}/unit_readHDF.c
    ${TEST_DIR}/test_suites.c
)

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
target_link_libraries(FY3G_Resampling_exe
    FY3G_Resampling
    ${HDF5_LIBRARIES}
    OpenMP::OpenMP_C
)

add_executable(FY3G_Resampling_test ${TEST_FILES})
target_include_directories(FY3G_Resampling_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/Unity/src
)
target_compile_definitions(FY3G_Resampling_test PRIVATE UNITY_INCLUDE_DOUBLE)
target_link_libraries(FY3G_Resampling_test
    FY3G_Resampling
    ${HDF5_LIBRARIES}
    OpenMP::OpenMP_C
)