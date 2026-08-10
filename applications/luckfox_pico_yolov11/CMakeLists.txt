cmake_minimum_required(VERSION 3.10)

# Define Toolchain

set(SDK_PATH "/home")
set(SDK_OUT_PATH "${SDK_PATH}/media/out/")
set(LIBC_TYPE "uclibc")

link_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/${LIBC_TYPE})

# OpenCV
set(OpenCV_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib/${LIBC_TYPE}/lib/cmake/opencv4")
find_package(OpenCV REQUIRED) # Define OpenCV_LIBS

set(CMAKE_C_COMPILER   "${SDK_PATH}/tools/linux/toolchain/arm-rockchip830-linux-uclibcgnueabihf/bin/arm-rockchip830-linux-uclibcgnueabihf-gcc")
set(CMAKE_CXX_COMPILER "${SDK_PATH}/tools/linux/toolchain/arm-rockchip830-linux-uclibcgnueabihf/bin/arm-rockchip830-linux-uclibcgnueabihf-g++")

# Define PROJECT_NAME

project(luckfox_pico_yolov11)

# Export Intellisense

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set Requried Flags

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath-link,${SDK_PATH}/media/out/lib")
set(CMAKE_INSTALL_RPATH "/oem/usr/lib:$ORIGIN")
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)

add_definitions(-DRV1106_1103)
# add_definitions(-DISABLE_LIBJPEG)

# Create .exe from SRCs

file(GLOB SRC_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/utils/file_utils.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/utils/image_utils.c"
)

# Library search paths

link_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/glibc
    ${SDK_OUT_PATH}/lib
)

add_executable(${PROJECT_NAME} ${SRC_FILES})

# Expose include directories

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/rknn
    ${CMAKE_CURRENT_SOURCE_DIR}/include/librga
    ${CMAKE_CURRENT_SOURCE_DIR}/include/stb
    ${CMAKE_CURRENT_SOURCE_DIR}/include/jpeg_turbo/include
    ${CMAKE_CURRENT_SOURCE_DIR}/utils
    
    ${SDK_OUT_PATH}
    ${SDK_OUT_PATH}/utils
    ${SDK_OUT_PATH}/common 
    ${SDK_OUT_PATH}/common/isp3.x   
    ${SDK_OUT_PATH}/include
    ${SDK_OUT_PATH}/include/rkaiq
    ${SDK_OUT_PATH}/include/rkaiq/uAPI2
    ${SDK_OUT_PATH}/include/rkaiq/common
    ${SDK_OUT_PATH}/include/rkaiq/xcore
    ${SDK_OUT_PATH}/include/rkaiq/algos
    ${SDK_OUT_PATH}/include/rkaiq/iq_parser
    ${SDK_OUT_PATH}/include/rkaiq/iq_parser_v2
    ${SDK_OUT_PATH}/include/rkaiq/smartIr
)

# Link any needed libraries to exe

target_link_libraries(${PROJECT_NAME}
    ${OpenCV_LIBS}
    sample_comm
    rtsp
    rkaiq
    rga
    rockit
    rockchip_mpp
    rknnmrt
    pthread
)