CXX       ?= g++
CXXFLAGS   = -std=c++17 -Wall -Wextra -O2 -I./src
BUILD_DIR  = build

# Prefer system fltk-config; optional local 1.4 tree overrides if present
LOCAL_FLTK := $(firstword \
	$(wildcard $(CURDIR)/fltk-1.4.*/bin/fltk-config) \
	$(wildcard $(CURDIR)/../fltk-1.4.*/bin/fltk-config))

ifeq ($(LOCAL_FLTK),)
  FLTK_CONFIG := fltk-config
else
  FLTK_CONFIG := bash $(LOCAL_FLTK)
endif

FLTK_CXX := $(shell $(FLTK_CONFIG) --cxxflags 2>/dev/null)

# Base FLTK libs only — do NOT use --use-images (embeds absolute distro paths)
FLTK_LD  := $(shell $(FLTK_CONFIG) --ldflags 2>/dev/null)

# SVG (Fl_SVG_Image) needs images + common image deps (portable -l names)
# Arch: pacman -S fltk libpng libjpeg-turbo
# Ubuntu: apt install libfltk1.3-dev (or local 1.4) libpng-dev libjpeg-dev
IMAGE_LIBS := -lfltk_images -lpng -ljpeg -lz

LIBS := -lsqlite3 -lzstd $(FLTK_LD) $(IMAGE_LIBS)

SRC = \
	src/main.cpp \
	src/graph/Graph.cpp \
	src/graph/UndoHistory.cpp \
	src/ui/GraphView.cpp \
	src/ui/RightPanel.cpp \
	src/storage/Storage.cpp

OBJ = $(patsubst src/%.cpp,$(BUILD_DIR)/%.o,$(SRC))
TARGET = $(BUILD_DIR)/WebThinker

.PHONY: all clean run dirs info

all: dirs $(TARGET)

dirs:
	@mkdir -p $(BUILD_DIR)/graph $(BUILD_DIR)/ui $(BUILD_DIR)/storage

info:
	@echo "FLTK_CONFIG = $(FLTK_CONFIG)"
	@echo "FLTK_CXX    = $(FLTK_CXX)"
	@echo "FLTK_LD     = $(FLTK_LD)"
	@echo "LIBS        = $(LIBS)"
	@$(FLTK_CONFIG) --version 2>/dev/null || echo "(no fltk-config)"

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)
	strip -s $@ 2>/dev/null || true
	@echo "Built $@"

$(BUILD_DIR)/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(FLTK_CXX) -c -o $@ $<

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(BUILD_DIR)
	rm -f *.tmp.db
