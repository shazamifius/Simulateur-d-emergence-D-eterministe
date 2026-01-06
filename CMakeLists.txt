cmake_minimum_required(VERSION 3.10)
project(SED_LAB CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_definitions(GRAPHICS_API_OPENGL_43)  # Request Compute Shader support

# --- Raylib Dependency ---
# Find the raylib package. With the vcpkg toolchain file,
# CMake will automatically find the raylib installed by vcpkg.
find_package(raylib CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# The following lines are commented out as they are related to the git submodule approach,
# which is replaced by the vcpkg dependency management.

# Workaround for Linux linker issues with vcpkg
if(UNIX AND NOT APPLE)
    link_directories($ENV{HOME}/vcpkg/installed/x64-linux/lib)
endif()

# set(BUILD_EXAMPLES OFF CACHE BOOL "Build raylib examples" FORCE)
# set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build raylib as a shared library" FORCE)
# add_subdirectory(libs/raylib)

# Find OpenMP for parallelism
find_package(OpenMP REQUIRED)
if(OpenMP_FOUND)
    message(STATUS "Found OpenMP, enabling parallel execution.")
endif()

# Define the executable and its source files
add_executable(sed_lab_final
    src/main.cpp
    src/MondeSED.cpp
    src/Interface.cpp
    src/Renderer.cpp
    src/Validator.cpp
    src/laws/LoiMetabolisme.cpp
    src/Chunk.cpp
    src/WorldMap.cpp
    src/InputSystem.cpp
    libs/imgui/imgui.cpp
    libs/imgui/imgui_draw.cpp
    libs/imgui/imgui_widgets.cpp
    libs/imgui/imgui_tables.cpp
    libs/imgui/imgui_demo.cpp
    libs/imgui/rlImGui.cpp
)

# Include directories for our project's headers
target_include_directories(sed_lab_final PUBLIC
    include
    libs/imgui
)

# Link against the 'raylib' target built from the submodule.
# This is much more robust. The raylib target will also bring its own dependencies (like GL, X11, etc.)
target_link_libraries(sed_lab_final PRIVATE
    raylib
    nlohmann_json::nlohmann_json
)

# Add platform-specific libraries for Linux to resolve linker issues.
if(UNIX AND NOT APPLE)
    target_link_libraries(sed_lab_final PRIVATE
        glfw3
        GL
        m
        pthread
        dl
        X11
    )
endif()

# Add OpenMP flags to our target
if(OpenMP_FOUND)
    target_link_libraries(sed_lab_final PRIVATE OpenMP::OpenMP_CXX)
endif()

# --- Verification Test ---
add_executable(test_determinism 
    tests/test_determinism.cpp 
    src/MondeSED.cpp 
    src/Validator.cpp 
    src/laws/LoiMetabolisme.cpp
    src/Chunk.cpp
    src/WorldMap.cpp
)
target_include_directories(test_determinism PUBLIC include libs/imgui) 
# Note: MondeSED depends on Raylib checks/logging maybe?
target_link_libraries(test_determinism PRIVATE raylib nlohmann_json::nlohmann_json)
if(OpenMP_FOUND)
    target_link_libraries(test_determinism PRIVATE OpenMP::OpenMP_CXX)
endif()


# --- Plugin System ---
add_library(healing_plugin SHARED plugins/SamplePlugin/HealingLaw.cpp)
target_include_directories(healing_plugin PUBLIC include)
target_compile_definitions(healing_plugin PRIVATE EXPORT_PLUGIN)
# Output to a specific 'plugins' folder in build dir for easy finding
set_target_properties(healing_plugin PROPERTIES 
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
)

add_executable(test_plugin_system tests/test_plugin_system.cpp src/utils/PluginManager.cpp)
target_include_directories(test_plugin_system PUBLIC include)
# On Linux we need dl
if(UNIX AND NOT APPLE)
    target_link_libraries(test_plugin_system PRIVATE dl)
endif()

# --- Unit Tests ---
add_executable(test_metabolism 
    tests/test_law_metabolism.cpp 
    src/laws/LoiMetabolisme.cpp
)
target_include_directories(test_metabolism PUBLIC include)

enable_testing()
add_test(NAME DeterminismTest COMMAND test_determinism)
add_test(NAME MetabolismLawTest COMMAND test_metabolism)
add_test(NAME PluginSystemTest COMMAND test_plugin_system $<TARGET_FILE:healing_plugin>)



