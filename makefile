CXX = g++
CXXFLAGS = -g -Wno-deprecated -DDEBUG -I compiler -Wall

EXE = cit
BUILD_DIR = build
MAIN_OBJ = $(BUILD_DIR)/main.o
OBJS = $(wildcard $(BUILD_DIR)/*.o)

compiler:
	@$(MAKE) -C compiler

$(MAIN_OBJ): main.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(EXE): $(MAIN_OBJ)
	$(CXX) $(OBJS) -o $@


all: compiler $(EXE)

clean:
	@rm -f $(EXE) $(OBJS) $(MAIN_OBJ)


.PHONY: all compiler
