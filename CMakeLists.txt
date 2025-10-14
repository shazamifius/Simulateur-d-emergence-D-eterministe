cmake_minimum_required(VERSION 3.10)
project(SED_LAB CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find OpenMP for parallelism
find_package(OpenMP REQUIRED)
if(OpenMP_FOUND)
    message(STATUS "Found OpenMP, enabling parallel execution.")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()

# Define the executable and its source files
add_executable(sed_lab
    app/main.cpp
    src/MondeSED.cpp
    imgui/imgui.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_widgets.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_demo.cpp
    imgui/rlImGui.cpp
)

# Include directories for headers
target_include_directories(sed_lab PUBLIC
    include
    imgui
)

# Link against raylib and other necessary libraries
# Note: This assumes raylib is installed in a standard system path.
# For a more robust setup, a FindRaylib.cmake module would be ideal.
target_link_libraries(sed_lab PRIVATE
    raylib
    GL
    m
    pthread
    dl
    rt
    X11
)

# Add OpenMP flags to the target
if(OpenMP_FOUND)
    target_link_libraries(sed_lab PRIVATE OpenMP::OpenMP_CXX)
endif()
