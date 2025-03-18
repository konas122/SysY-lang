AR = ar
ARFLAGS = rcs

CXX = g++
CXXFLAGS = -g -std=c++17 -Wno-deprecated -DDEBUG -I compiler -Wall

NPROC = $(shell nproc)

EXE = cit
BUILD_DIR = build
MAIN_OBJ = $(BUILD_DIR)/main.o

STATIC_OBJS = $(BUILD_DIR)/compiler.a $(BUILD_DIR)/assembler.a $(BUILD_DIR)/linker.a
OBJS = $(MAIN_OBJ) $(STATIC_OBJS)


compiler:
	@if [ ! -d $(BUILD_DIR) ]; then mkdir $(BUILD_DIR); fi
	@if [ ! -d $(BUILD_DIR)/compiler ]; then mkdir $(BUILD_DIR)/compiler; fi
	@$(MAKE) -C compiler -j $(NPROC)
	@echo

assembler:
	@if [ ! -d $(BUILD_DIR)/assembler ]; then mkdir $(BUILD_DIR)/assembler; fi
	@$(MAKE) -C assembler -j $(NPROC)
	@echo

linker:
	@if [ ! -d $(BUILD_DIR)/linker ]; then mkdir $(BUILD_DIR)/linker; fi
	@$(MAKE) -C linker -j $(NPROC)
	@echo


$(BUILD_DIR)/%.a: $(BUILD_DIR)/%/*.o
	@$(AR) $(ARFLAGS) $@ $^
	@echo "    AR    " $(notdir $@)

$(MAIN_OBJ): main.cpp
	@$(CXX) $(CXXFLAGS) -c $< -o $@
	@echo "    CPP   " $(notdir $@)

$(EXE): $(OBJS)
	@$(CXX) $(OBJS) -o $@
	@echo "    LD    " $@


all: compiler assembler linker $(EXE)

clean:
	@rm -f $(EXE)
	@rm -rf $(BUILD_DIR)/*


.PHONY: all compiler assembler linker
