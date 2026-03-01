# Simple Makefile for pattern_finder project
CXX = g++
CXXFLAGS = -std=c++17 -O2 -g -Wall -Wextra -Wpedantic
INCLUDES = -Iinclude -I/usr/local/anaconda3/include
LIBS = -lboost_program_options

# Source files
SRCDIR = src
SOURCES = $(wildcard $(SRCDIR)/*.cpp)
OBJECTS = $(SOURCES:.cpp=.o)
TARGET = pattern_finder

# Default target
all: $(TARGET)

# Build the executable
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^ $(LIBS)

# Compile source files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(OBJECTS) $(TARGET)

# Install dependencies (for reference)
deps:
	@echo "Dependencies needed: json-c-devel boost-devel"
	@echo "On RHEL/CentOS: sudo yum install json-c-devel boost-devel"
	@echo "On Ubuntu/Debian: sudo apt install libjson-c-dev libboost-program-options-dev"

.PHONY: all clean deps
