# Build LRPC test application
cmake_minimum_required(VERSION 3.16)
project(lrpc_test)

include(GNUInstallDirs)

find_package(iceoryx_platform REQUIRED)
find_package(iceoryx_posh CONFIG REQUIRED)
find_package(iceoryx_hoofs CONFIG REQUIRED)

include(IceoryxPackageHelper)
include(IceoryxPlatform)
include(IceoryxPlatformSettings)

# Add include directory
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    /usr/local/includle/iceoryx/
)

iox_add_executable(
    TARGET  lrpc_test
    FILES   test/test_lrpc.cpp
    LIBS    iceoryx_posh::iceoryx_posh
)
