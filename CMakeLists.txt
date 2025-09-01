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
    /usr/local/include/iceoryx/
)

iox_add_executable(
    TARGET  test_lrpc_client
    FILES   test/test_lrpc_client.cc
    LIBS    iceoryx_posh::iceoryx_posh
)

iox_add_executable(
    TARGET  test_lrpc_server
    FILES   test/test_lrpc_server.cc
    LIBS    iceoryx_posh::iceoryx_posh
)