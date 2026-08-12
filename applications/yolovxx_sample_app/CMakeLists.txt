cmake_minimum_required(VERSION 3.10)

# 1. Toolchain Setup (Required for cross-compiling to ARM)
set(SDK_PATH "/home")
set(SDK_OUT_PATH "${SDK_PATH}/media/out/")
set(LIBC_TYPE "uclibc")

link_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/${LIBC_TYPE})

set(CMAKE_C_COMPILER   "${SDK_PATH}/tools/linux/toolchain/arm-rockchip830-linux-uclibcgnueabihf/bin/arm-rockchip830-linux-uclibcgnueabihf-gcc")
set(CMAKE_CXX_COMPILER "${SDK_PATH}/tools/linux/toolchain/arm-rockchip830-linux-uclibcgnueabihf/bin/arm-rockchip830-linux-uclibcgnueabihf-g++")

project(yolovxx_sample_app)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath-link,${SDK_PATH}/media/out/lib")
set(CMAKE_INSTALL_RPATH "/oem/usr/lib:$ORIGIN")
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)

set(OpenCV_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib/${LIBC_TYPE}/lib/cmake/opencv4")
find_package(OpenCV REQUIRED) # Define OpenCV_LIBS

# 3. Board Shared Library Search Paths
link_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/lib
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/${LIBC_TYPE}
    ${SDK_OUT_PATH}/lib
)

add_executable(${PROJECT_NAME} src/main.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/yolovxx
    ${SDK_OUT_PATH}/include/rkaiq
    ${SDK_OUT_PATH}/include/rkaiq/uAPI2
    ${SDK_OUT_PATH}/include/rkaiq/common
    ${SDK_OUT_PATH}/include/rkaiq/xcore
    ${SDK_OUT_PATH}/include/rkaiq/algos
    ${SDK_OUT_PATH}/include/rkaiq/iq_parser
    ${SDK_OUT_PATH}/include/rkaiq/iq_parser_v2
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/libyolovxx_lib.a
    ${OpenCV_LIBS}
    rknnmrt
    rockit 
    rga
    rkaiq
    sample_comm
    rtsp
)