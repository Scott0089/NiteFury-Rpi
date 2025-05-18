# Auto-generated CMakeLists.txt
cmake_minimum_required(VERSION 3.2)

# Library name based on directory name: 
file(GLOB_RECURSE XLNX_LIB_SRC
    tmrctr/*.c
    gpio/*.c
    standalone/*.c
    sysmon/*.c
    bram/*.c
)

#define library 
add_library(xlnx_libs STATIC ${XLNX_LIB_SRC})

# Link
target_link_libraries(xlnx_libs m)

# Add include directories
target_include_directories(xlnx_libs PUBLIC
    gpio
    standalone
    hw_config
    tmrctr
    sysmon
    bram
    ../sc_libs
    ../
)