cmake_minimum_required(VERSION 3.10)

set(SDK_PATH "/home")
set(SDK_OUT_PATH "${SDK_PATH}/media/out")
set(LIBC_TYPE "uclibc")

link_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/${LIBC_TYPE})

set(USE_YOLO_VERSION "10" CACHE STRING "YOLO model version to compile (5, 10, 11)")
message(STATUS "Configuring build for YOLO version: ${USE_YOLO_VERSION}")

set(OpenCV_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib/${LIBC_TYPE}/lib/cmake/opencv4")
find_package(OpenCV REQUIRED) # Define OpenCV_LIBS

set(CMAKE_C_COMPILER   "${SDK_PATH}/tools/linux/toolchain/arm-rockchip830-linux-uclibcgnueabihf/bin/arm-rockchip830-linux-uclibcgnueabihf-gcc")
set(CMAKE_CXX_COMPILER "${SDK_PATH}/tools/linux/toolchain/arm-rockchip830-linux-uclibcgnueabihf/bin/arm-rockchip830-linux-uclibcgnueabihf-g++")

# Define PROJECT_NAME

project(yolovxx_sample)

# Export Intellisense

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set Requried Flags

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath-link,${SDK_PATH}/media/out/lib")
set(CMAKE_INSTALL_RPATH "/oem/usr/lib:$ORIGIN")
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)

add_definitions(-DRV1106_1103)
add_definitions(-DDISABLE_LIBJPEG)

set(COMMON_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/rknn
    ${CMAKE_CURRENT_SOURCE_DIR}/yolo_zoo/${USE_YOLO_VERSION}
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/librga
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/stb_image
    ${CMAKE_CURRENT_SOURCE_DIR}/utils
    ${SDK_OUT_PATH}/include/rkaiq
    ${SDK_OUT_PATH}/include/rkaiq/uAPI2
    ${SDK_OUT_PATH}/include/rkaiq/common
    ${SDK_OUT_PATH}/include/rkaiq/xcore
    ${SDK_OUT_PATH}/include/rkaiq/algos
    ${SDK_OUT_PATH}/include/rkaiq/iq_parser
    ${SDK_OUT_PATH}/include/rkaiq/iq_parser_v2
)

file(GLOB YOLO_VER_SRCS
    "yolo_zoo/${USE_YOLO_VERSION}/*.cc"
)

add_library(yolovxx_lib STATIC 
    yolovxx/yolovxx_ai.cpp 
    yolovxx/yolovxx_media.cpp
    yolovxx/yolovxx_util.cpp
    yolovxx/luckfox_mpi.cc
    ${YOLO_VER_SRCS} # VERSION SPECIFIC
    
    utils/file_utils.c
    utils/image_utils.c
)

set_target_properties(yolovxx_lib PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_compile_definitions(yolovxx_lib PUBLIC YOLO_VERSION=${USE_YOLO_VERSION})

target_include_directories(yolovxx_lib PUBLIC
    ${COMMON_INCLUDE_DIRS}
)

# Library search paths

target_link_libraries(yolovxx_lib
    ${OpenCV_LIBS}
    rknnmrt
    rockit 
    rga
    rkaiq
    sample_comm
    rtsp
)