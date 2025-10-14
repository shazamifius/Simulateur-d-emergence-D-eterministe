cmake_minimum_required(VERSION 3.10)
project(SED_LAB CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Raylib Dependency ---
# Find the raylib package. With the vcpkg toolchain file,
# CMake will automatically find the raylib installed by vcpkg.
find_package(raylib CONFIG REQUIRED)

# The following lines are commented out as they are related to the git submodule approach,
# which is replaced by the vcpkg dependency management.
# set(BUILD_EXAMPLES OFF CACHE BOOL "Build raylib examples" FORCE)
# set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build raylib as a shared library" FORCE)
# add_subdirectory(libs/raylib)

# Find OpenMP for parallelism
find_package(OpenMP REQUIRED)
if(OpenMP_FOUND)
    message(STATUS "Found OpenMP, enabling parallel execution.")
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

# Include directories for our project's headers
target_include_directories(sed_lab PUBLIC
    include
    imgui
)

# Link against the 'raylib' target built from the submodule.
# This is much more robust. The raylib target will also bring its own dependencies (like GL, X11, etc.)
target_link_libraries(sed_lab PRIVATE
    raylib
    glfw3
    GL
    m
    pthread
    dl
    X11
)

# Add OpenMP flags to our target
if(OpenMP_FOUND)
    target_link_libraries(sed_lab PRIVATE OpenMP::OpenMP_CXX)
endif()
