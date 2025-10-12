# Makefile for SED Simulator

# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -std=c++14 -Wall -fopenmp

# Source files
SRCS = main.cpp MondeSED.cpp

# Object files
OBJS = $(SRCS:.cpp=.o)

# Executable name
TARGET = sed_simulator

# Default rule
all: $(TARGET)

# Rule to link the executable
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

# Rule to compile source files into object files
.cpp.o:
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean rule
clean:
	rm -f $(OBJS) $(TARGET)

# Phony targets
.PHONY: all clean