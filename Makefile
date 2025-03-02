# Compiler and flags
CXXFLAGS = -std=c++17 # Add C++17 flag here
CXX := g++
CXXFLAGS := -Isrc/Include
LDFLAGS := -Lsrc/lib
LIBS := -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf

# Target executable
TARGET := main
SRC := text.cc

# Build rule
$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $(TARGET) $(SRC) $(LIBS)

# Clean rule
clean:
	rm -f $(TARGET)
