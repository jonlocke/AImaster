# Compiler and flags
CXX = cl 
CXXFLAGS = -Wall -std=c++17

# Output executable
TARGET = main.exe

# Source files
SRCS = main.cpp

# Default target
all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRCS)
clean:
	del /Q $(TARGET)

.PHONY: all