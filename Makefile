# Makefile for MyGit project

# Compiler
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -lz

# Target executable
TARGET = mygit

# Source files
SRCS = mygit.cpp

# Build target
all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET)

# Clean target
clean:
	rm -f $(TARGET)

# Run target
run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
