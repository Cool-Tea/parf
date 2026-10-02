ROOT 			= $(shell pwd)
INC_DIR	 	= $(ROOT)/include
BUILD_DIR = $(ROOT)/build
EG_DIR 		= $(ROOT)/example

EXAMPLE_SRCS  = $(shell find $(EG_DIR) -name "*.cpp" -type f)
EXAMPLES		  = $(EXAMPLE_SRCS:$(EG_DIR)/%.cpp=%)
BINS          = $(EXAMPLE_SRCS:$(EG_DIR)/%.cpp=$(BUILD_DIR)/%)

CXX			  = g++
CXXFLAGS := -I$(INC_DIR) -std=c++26 -freflection -Wall -Wextra -Wpedantic -O2

all: $(BINS)

$(EXAMPLES): %: $(BUILD_DIR)/%

$(BUILD_DIR)/%: $(EG_DIR)/%.cpp $(INC_DIR)/parf/parf.hpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean