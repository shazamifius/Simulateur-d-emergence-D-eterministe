# Makefile for the SED-Lab C++ Edition

# --- Variables ---
# Compiler
CXX = g++

# Directories
SRC_DIRS = src imgui app
INCLUDE_DIRS = include imgui
BUILD_DIR = build

# --- Compiler and Linker Flags ---
# CXXFLAGS:
# -std=c++17: Use the C++17 standard for modern features.
# -Wall -Wextra: Enable extensive warnings.
# -O3: High optimization level.
# -g: Include debug symbols.
# -fopenmp: Enable OpenMP for multi-threading in simulation logic.
# -I<dir>: Add directories to the header search path.
# -DPLATFORM_DESKTOP: Flag for raylib build.
CXXFLAGS = -std=c++17 -Wall -Wextra -O3 -g -fopenmp -DPLATFORM_DESKTOP
CXXFLAGS += $(foreach dir,$(INCLUDE_DIRS),-I$(dir))
CXXFLAGS += -I/usr/local/include # Common path for system-wide libraries
CXXFLAGS += -I/usr/include # Standard path for dev libraries like raylib

# LDFLAGS:
# Link against required libraries: raylib, OpenGL, math, etc.
LDFLAGS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11 -fopenmp
LDFLAGS += -L/usr/local/lib # Common path for system-wide libraries

# --- Source and Object Files ---
# Find all .cpp files in the specified source directories
SRCS = $(foreach dir,$(SRC_DIRS),$(wildcard $(dir)/*.cpp))

# Generate corresponding object (.o) file names in the build directory
OBJS = $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(notdir $(SRCS)))

# --- Application ---
APP_NAME = sed_lab
APP_TARGET = $(APP_NAME)

# --- Targets ---

# Default target: builds the executable
all: $(APP_TARGET)

# Rule to link the final executable
$(APP_TARGET): $(OBJS)
	@echo "==> Linking final executable: $@"
	$(CXX) -o $@ $(OBJS) $(LDFLAGS)
	@echo "==> Build complete. Executable is '$(APP_TARGET)'."

# Use VPATH to specify search paths for prerequisites
VPATH = $(SRC_DIRS)

# Generic rule to compile object files into the build directory
$(BUILD_DIR)/%.o: %.cpp
	@echo "==> Compiling source: $<"
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Phony targets do not represent files
.PHONY: all clean run help

# Target to clean up the directory
clean:
	@echo "==> Cleaning up build artifacts..."
	rm -f $(APP_TARGET) $(OBJS)
	rm -rf $(BUILD_DIR)
	@echo "==> Cleanup complete."

# Target to run the application
run: all
	@echo "==> Launching SED-Lab with virtual display..."
	xvfb-run --auto-servernum --server-args="-screen 0 1280x800x24" ./$(APP_TARGET)

# Target to display help
help:
	@echo "Available targets:"
	@echo "  all     - (Default) Compiles the SED-Lab application."
	@echo "  run     - Compiles and launches the application."
	@echo "  clean   - Removes all compiled files."
	@echo "  help    - Displays this help message."