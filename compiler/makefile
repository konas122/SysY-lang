CXX = g++
CXXFLAGS = -g -Wno-deprecated -DDEBUG -I . -Wall
BUILD_DIR = ../build

SRC = $(shell find . -name '*.cpp')
FILENAME = $(notdir $(SRC))
OBJS = $(patsubst %.cpp, $(BUILD_DIR)/%.o, $(FILENAME))


$(BUILD_DIR)/%.o: %.cpp
	@$(CXX) $(CXXFLAGS) -c $< -o $@
	@echo "    CPP   " $(notdir $@)

$(BUILD_DIR)/%.o: ir/%.cpp
	@$(CXX) $(CXXFLAGS) -c $< -o $@
	@echo "    CPP   " $(notdir $@)

$(BUILD_DIR)/%.o: lexer/%.cpp
	@$(CXX) $(CXXFLAGS) -c $< -o $@
	@echo "    CPP   " $(notdir $@)

$(BUILD_DIR)/%.o: optimize/%.cpp
	@$(CXX) $(CXXFLAGS) -c $< -o $@
	@echo "    CPP   " $(notdir $@)

$(BUILD_DIR)/%.o: parser/%.cpp
	@$(CXX) $(CXXFLAGS) -c $< -o $@
	@echo "    CPP   " $(notdir $@)

$(BUILD_DIR)/%.o: symbol/%.cpp
	@$(CXX) $(CXXFLAGS) -c $< -o $@
	@echo "    CPP   " $(notdir $@)


all: $(OBJS)


.PHONY: all
