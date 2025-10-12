# Makefile for the SED Simulator project

# --- Variables ---
# Compiler
CXX = g++

# Compiler flags:
# -std=c++14: Use the C++14 standard
# -Wall: Enable all warnings
# -O3: Maximum optimization level
# -fopenmp: Enable OpenMP for parallelization
CXXFLAGS = -std=c++14 -Wall -O3 -fopenmp

# Linker flags:
# -fopenmp: Link against the OpenMP library
LDFLAGS = -fopenmp

# Executable name
TARGET = sed_simulator

# Source files
SRCS = main.cpp MondeSED.cpp

# Object files (derived from source files)
OBJS = $(SRCS:.cpp=.o)

# --- Targets ---

# Default target: builds the executable
all: $(TARGET)

# Rule to link the final executable
$(TARGET): $(OBJS)
	@echo "Linking executable: $@"
	$(CXX) $(LDFLAGS) -o $@ $(OBJS)
	@echo "Build complete. Executable is '$(TARGET)'."

# Rule to compile .cpp files into .o files
# Depends on the corresponding .cpp file and the main header
%.o: %.cpp MondeSED.h
	@echo "Compiling: $<"
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Phony targets do not represent files
.PHONY: all clean run help

# Target to clean up the directory
clean:
	@echo "Cleaning up build artifacts..."
	rm -f $(TARGET) $(OBJS) *.o
	@echo "Cleanup complete."

# Target to run a default simulation for quick testing
run: all
	@echo "Running a default simulation (16x16x16, 50 cycles, 10% density)..."
	./$(TARGET) 16 16 16 50 0.1 default_sim

# Target to display help
help:
	@echo "Available targets:"
	@echo "  all     - (Default) Compiles the project."
	@echo "  run     - Compiles and runs a default simulation."
	@echo "  clean   - Removes all compiled files."
	@echo "  help    - Displays this help message."