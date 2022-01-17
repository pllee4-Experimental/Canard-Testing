add_library(canard 
    libcanard/canard.c
    socketcan/socketcan.c)
target_include_directories(canard PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/libcanard>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/socketcan>
    $<INSTALL_INTERFACE:include>)
