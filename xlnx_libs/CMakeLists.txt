# Auto-generated CMakeLists.txt
cmake_minimum_required(VERSION 3.2)

# Library name based on directory name: 
file(GLOB_RECURSE XLNX_LIB_SRC
    gpio/*.c
    standalone/*.c
)

#define library 
add_library(xlnx_libs STATIC ${XLNX_LIB_SRC})

# Link to math and SDL2 (SDL2 might not be needed)
target_link_libraries(xlnx_libs m)

# Add include directories
target_include_directories(xlnx_libs PUBLIC
    gpio
    standalone
    hw_config
    ../sc_libs
    ../
)
