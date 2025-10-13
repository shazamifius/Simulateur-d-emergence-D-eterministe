# Makefile for the SED Simulator project (Refactored for Library Structure)

# --- Variables ---
# Compiler
CXX = g++

# Directories
APP_DIR = app
SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build
LIB_DIR = lib

# --- Compiler and Linker Flags ---
# CXXFLAGS:
# -std=c++14: Use the C++14 standard.
# -Wall: Enable all standard compiler warnings.
# -Wextra: Enable extra warnings.
# -O3: Highest level of optimization.
# -g: Include debugging symbols.
# -fopenmp: Enable OpenMP for multi-threading.
# -I<dir>: Tell the compiler where to find header files.
# -MMD -MP: Generate dependency files (.d) for header tracking.
CXXFLAGS = -std=c++14 -Wall -Wextra -O3 -g -fopenmp -I$(INCLUDE_DIR) -MMD -MP

# LDFLAGS:
# -fopenmp: Link against the OpenMP library.
LDFLAGS = -fopenmp

# --- Source and Object Files ---
# Automatically find all .cpp files in the source directories
LIB_SRCS = $(wildcard $(SRC_DIR)/*.cpp)
APP_SRCS = $(wildcard $(APP_DIR)/*.cpp)

# Generate corresponding object (.o) and dependency (.d) file names
LIB_OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(LIB_SRCS))
APP_OBJS = $(patsubst $(APP_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(APP_SRCS))
DEPS = $(LIB_OBJS:.o=.d) $(APP_OBJS:.o=.d)

# --- Library ---
LIB_NAME = sed
LIB_TARGET = $(LIB_DIR)/lib$(LIB_NAME).a

# --- Application ---
APP_NAME = sed_simulator
APP_TARGET = $(APP_NAME)

# --- Targets ---

# Default target: builds the executable
all: $(APP_TARGET)

# Rule to link the final executable
$(APP_TARGET): $(APP_OBJS) $(LIB_TARGET)
	@echo "==> Linking executable: $@"
	$(CXX) $(LDFLAGS) -o $@ $(APP_OBJS) -L$(LIB_DIR) -l$(LIB_NAME)
	@echo "==> Build complete. Executable is '$(APP_TARGET)'."

# Rule to create the static library from its object files
$(LIB_TARGET): $(LIB_OBJS)
	@echo "==> Archiving static library: $@"
	@mkdir -p $(LIB_DIR)
	ar rcs $@ $(LIB_OBJS)

# Generic rule to compile object files into the build directory
# This single rule handles both library and application sources
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@echo "==> Compiling library source: $<"
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(APP_DIR)/%.cpp
	@echo "==> Compiling application source: $<"
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Phony targets do not represent files
.PHONY: all clean run help

# Target to clean up the directory
clean:
	@echo "==> Cleaning up build artifacts..."
	rm -f $(APP_TARGET) $(APP_OBJS) $(LIB_OBJS) $(DEPS)
	rm -rf $(BUILD_DIR) $(LIB_DIR)
	@echo "==> Cleanup complete."

# Target to run a default simulation for quick testing
run: all
	@echo "==> Running a default simulation (16x16x16, 50 cycles, 10% density)..."
	./$(APP_TARGET) 16 16 16 50 0.1 default_sim

# Target to display help
help:
	@echo "Available targets:"
	@echo "  all     - (Default) Compiles the project robustly."
	@echo "  run     - Compiles and runs a default simulation."
	@echo "  clean   - Removes all compiled files and dependencies."
	@echo "  help    - Displays this help message."

# Include the generated dependency files.
# The '-' before 'include' tells make to ignore errors if the file doesn't exist.
-include $(DEPS)