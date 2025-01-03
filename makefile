CXX = g++
CXXFLAGS = -g -Wno-deprecated -DDEBUG -I compiler -Wall

EXE = cit
NPROC = $(shell nproc)
BUILD_DIR = build
MAIN_OBJ = $(BUILD_DIR)/main.o

SRC = $(notdir $(shell find compiler assembler linker -name '*.cpp'))
OBJS = $(patsubst %.cpp, $(BUILD_DIR)/%.o, $(SRC))
OBJS += $(MAIN_OBJ)


compiler:
	@$(MAKE) -C compiler -j $(NPROC)
	@echo

$(MAIN_OBJ): main.cpp
	@$(CXX) $(CXXFLAGS) -c $< -o $@
	@echo "    CPP   " $(notdir $@)

$(EXE): $(OBJS)
	@$(CXX) $(OBJS) -o $@
	@echo "    LD    " $@


all: compiler $(EXE)

clean:
	@rm -f $(EXE) $(OBJS) $(MAIN_OBJ)


.PHONY: all compiler
