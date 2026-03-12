.PHONY: all clean rebuild debug

CXX ?= g++

SRC_DIR := src
INCLUDE_DIR := include
LIB_DIR := lib
BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj
BIN_DIR := $(BUILD_DIR)/bin
TARGET := $(BIN_DIR)/GtkAppFolder

# Enable parallel builds by default; override with `make JOBS=4`.
JOBS ?= $(shell nproc)
ifeq ($(MAKELEVEL),0)
ifneq ($(filter -j% --jobs%,$(MAKEFLAGS)),)
else
MAKEFLAGS += -j$(JOBS)
endif
endif

# Automatically discover all C++ source files under src/.
SRC_FILES := $(shell find $(SRC_DIR) -type f -name '*.cpp')
OBJ_FILES := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRC_FILES))
DEP_FILES := $(OBJ_FILES:.o=.d)

CPPFLAGS := -I$(INCLUDE_DIR) -I$(SRC_DIR)
CXXFLAGS_COMMON := -std=c++17 -Wall -Wextra
CXXFLAGS_RELEASE := -O2
CXXFLAGS_DEBUG := -O0 -g

BUILD ?= release
ifeq ($(BUILD),debug)
	CXXFLAGS := $(CXXFLAGS_COMMON) $(CXXFLAGS_DEBUG)
else
	CXXFLAGS := $(CXXFLAGS_COMMON) $(CXXFLAGS_RELEASE)
endif

# Link all libraries matching lib/lib*.a and lib/lib*.so.
LIB_SEARCH_PATHS := -L$(LIB_DIR)
LIB_NAMES := $(notdir $(basename $(wildcard $(LIB_DIR)/lib*.a $(LIB_DIR)/lib*.so)))
LDLIBS := $(patsubst lib%,-l%,$(LIB_NAMES))
LDFLAGS += $(LIB_SEARCH_PATHS)

ifeq ($(strip $(SRC_FILES)),)
all:
	@echo "Error: no C++ source files found under $(SRC_DIR)/"
	@echo "Add .cpp files to $(SRC_DIR)/ or update source discovery in Makefile."
	@exit 1
else
all: $(TARGET)

$(TARGET): $(OBJ_FILES) | $(BIN_DIR)
	$(CXX) $(OBJ_FILES) $(LDFLAGS) $(LDLIBS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

$(BIN_DIR):
	@mkdir -p $@

clean:
	rm -rf $(BUILD_DIR)

rebuild: clean all

debug:
	$(MAKE) BUILD=debug rebuild

endif

-include $(DEP_FILES)
