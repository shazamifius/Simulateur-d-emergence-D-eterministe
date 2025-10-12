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

# Compiler flags
# -I<dir>: Tell the compiler where to find header files
CXXFLAGS = -std=c++14 -Wall -O3 -fopenmp -I$(INCLUDE_DIR)

# Linker flags
LDFLAGS = -fopenmp

# --- Library ---
LIB_NAME = sed
LIB_TARGET = $(LIB_DIR)/lib$(LIB_NAME).a
LIB_SRCS = $(SRC_DIR)/MondeSED.cpp
LIB_OBJS = $(BUILD_DIR)/MondeSED.o

# --- Application ---
APP_NAME = sed_simulator
APP_TARGET = $(APP_NAME)
APP_SRCS = $(APP_DIR)/main.cpp
APP_OBJS = $(BUILD_DIR)/main.o

# --- Targets ---

# Default target: builds the executable
all: $(APP_TARGET)

# Rule to link the final executable
$(APP_TARGET): $(APP_OBJS) $(LIB_TARGET)
	@echo "Linking executable: $@"
	$(CXX) $(LDFLAGS) -o $@ $(APP_OBJS) -L$(LIB_DIR) -l$(LIB_NAME)
	@echo "Build complete. Executable is '$(APP_TARGET)'."

# Rule to create the static library from its object files
$(LIB_TARGET): $(LIB_OBJS)
	@echo "Archiving static library: $@"
	@mkdir -p $(LIB_DIR)
	ar rcs $@ $(LIB_OBJS)

# Rule to compile the application's object file
$(APP_OBJS): $(APP_SRCS)
	@echo "Compiling application: $<"
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Rule to compile the library's object file
$(LIB_OBJS): $(LIB_SRCS)
	@echo "Compiling library: $<"
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Phony targets do not represent files
.PHONY: all clean run help

# Target to clean up the directory
clean:
	@echo "Cleaning up build artifacts..."
	rm -f $(APP_TARGET) $(APP_OBJS) $(LIB_OBJS)
	rm -rf $(BUILD_DIR) $(LIB_DIR)
	@echo "Cleanup complete."

# Target to run a default simulation for quick testing
run: all
	@echo "Running a default simulation (16x16x16, 50 cycles, 10% density)..."
	./$(APP_TARGET) 16 16 16 50 0.1 default_sim

# Target to display help
help:
	@echo "Available targets:"
	@echo "  all     - (Default) Compiles the project."
	@echo "  run     - Compiles and runs a default simulation."
	@echo "  clean   - Removes all compiled files and the library."
	@echo "  help    - Displays this help message."