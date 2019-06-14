# Makefile for Basler pylon sample program
.PHONY: makeoutdir all movetooutdir cleano cleanup clean

# The program to build
NAME       := PylonSample_Stereo_Acquisition_PTP

# We will make an output directory to hold the final builds
MKDIR_P    := mkdir -p
OUT_DIR    := ./bin_linux

# Installation directories for pylon
PYLON_ROOT := /opt/pylon5

# Additional libraries, etc.
OPENCV_LIB := $(shell pkg-config --libs opencv)
OPENCV_INC := /usr/include/opencv2

# Build tools and flags
SRC_DIR    := ./source
INC_DIR    := ./include
OBJ_DIR    := $(OUT_DIR)
SRC_FILES  := $(wildcard $(SRC_DIR)/*.cpp)
OBJ_FILES  := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRC_FILES))
LD         := $(CXX)
CPPFLAGS   := $(shell $(PYLON_ROOT)/bin/pylon-config --cflags) -std=c++11 -I$(INC_DIR) -I$(OPENCV_INC)
CXXFLAGS   := #e.g., CXXFLAGS=-g -O0 for debugging
LDFLAGS    := $(shell $(PYLON_ROOT)/bin/pylon-config --libs-rpath)
LDLIBS     := $(shell $(PYLON_ROOT)/bin/pylon-config --libs) $(OPENCV_LIB)

# Rules for building: make output directory, make program, move to output directory
all: makeoutdir cleano $(NAME) movetooutdir cleanup

makeoutdir:
	${MKDIR_P} $(OUT_DIR)

movetooutdir:
	mv $(NAME) $(OUT_DIR)

cleano:
	rm -f $(OUT_DIR)/*.o

cleanup:
	rm -f $(OUT_DIR)/*.o

$(NAME): $(OBJ_FILES)
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS) -lpthread

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

#all: $(NAME)

#$(NAME): $(NAME).o
#	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS) -lpthread

#$(NAME).o: $(NAME).cpp
#	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

clean:
	# $(RM) $(NAME).o $(NAME)
	rm -rf $(OUT_DIR)
