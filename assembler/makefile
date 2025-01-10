CXX = g++
CXXFLAGS = -g -Wno-deprecated -DDEBUG -I . -I .. -Wall
BUILD_DIR = ../build/assembler

SRC = $(shell find . -type f \( -name '*.cpp' -o -name '*.c' \))
FILENAME = $(notdir $(SRC))
OBJS = $(patsubst %.c, $(BUILD_DIR)/%.o, $(FILENAME))
OBJS += $(patsubst %.cpp, $(BUILD_DIR)/%.o, $(FILENAME))


$(BUILD_DIR)/%.o: %.c
	@$(CXX) $(CXXFLAGS) -c $< -o $@
	@echo "    CPP   " $(notdir $@)

$(BUILD_DIR)/%.o: %.cpp
	@$(CXX) $(CXXFLAGS) -c $< -o $@
	@echo "    CPP   " $(notdir $@)


all: $(OBJS)


.PHONY: all
