cmake_minimum_required(VERSION 3.10)
project(SED_LAB CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Raylib Submodule ---
# Instruct the raylib build system to not build the examples, and build as a static library.
set(BUILD_EXAMPLES OFF CACHE BOOL "Build raylib examples" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build raylib as a shared library" FORCE)
# Add the raylib submodule directory to the build.
# This will find its own CMakeLists.txt and create the 'raylib' target.
add_subdirectory(libs/raylib)

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
)

# Add OpenMP flags to our target
if(OpenMP_FOUND)
    target_link_libraries(sed_lab PRIVATE OpenMP::OpenMP_CXX)
endif()
