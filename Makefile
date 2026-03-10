# Makefile for Chess Game with SFML

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -fopenmp
CPPFLAGS = -Iinclude
LDFLAGS = -lsfml-graphics -lsfml-window -lsfml-system -fopenmp

# Source and object files
SRC_DIR = src
OBJ_DIR = build/obj

SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SOURCES))
DEPFILES = $(OBJECTS:.o=.d)
EXECUTABLE = ChessGame

.PHONY: all clean run rebuild

all: $(EXECUTABLE)

$(OBJ_DIR):
	mkdir -p $@

$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Build complete: $(EXECUTABLE)"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -MMD -MP -c $< -o $@

run: $(EXECUTABLE)
	./$(EXECUTABLE)

clean:
	rm -rf build/ $(EXECUTABLE)
	@echo "Clean complete"

rebuild: clean all

-include $(DEPFILES)
