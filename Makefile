# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -Wall -Wextra -pedantic -std=c++17 -g

# Source files
SRCS = $(wildcard $(SOURCE_DIR)/*.cpp)

# Object files
OBJS = $(patsubst $(SOURCE_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRCS))

# Executable name
EXEC = self-tftp

# Directories
BUILD_DIR = build
SOURCE_DIR = source

# Build executable
$(EXEC): $(OBJS)
	@$(CXX) $(CXXFLAGS) $(OBJS) -o $(EXEC)

# Compile source files
$(BUILD_DIR)/%.o: $(SOURCE_DIR)/%.cpp
	@$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up object files and executable
clean:
	@rm -f $(OBJS) $(EXEC)