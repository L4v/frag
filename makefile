SHELL      = /bin/bash
BUILD_SUCCESS = '\033[1;32m'Build complete for Linux'\033[0m'

BUILD_DIR  = ./build
SOURCE_DIR = ./code
IMGUI_DIR  = $(SOURCE_DIR)/include/imgui
GLAD_DIR   = $(SOURCE_DIR)/libs

EXE      = frag
SOURCES  = $(wildcard $(SOURCE_DIR)/*.cpp)
SOURCES += $(wildcard $(IMGUI_DIR)/*.cpp)
OBJS     = $(addprefix $(BUILD_DIR)/, $(addsuffix .o, $(basename $(notdir $(SOURCES)))))

CC       = g++
CFLAGS   = -I$(SOURCE_DIR)/include -g
CFLAGS  += -DIMGUI_IMPL_OPENGL_LOADER_GLAD
LDFLAGS  = -ldl
LDFLAGS += `pkg-config --libs glfw3 assimp`


$(BUILD_DIR)/%.o: $(SOURCE_DIR)/%.cpp
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/%.o: $(IMGUI_DIR)/%.cpp
	$(CC) $(CFLAGS) -c -o $@ $<

all: $(BUILD_DIR)/$(EXE)
	@echo -e $(BUILD_SUCCESS)

$(BUILD_DIR)/$(EXE): $(OBJS)
	$(CC) -o $@ $^ $(GLAD_DIR)/glad.c $(CFLAGS) $(LDFLAGS)

clean:
	rm -f $(BUILD_DIR)/$(EXE) $(OBJS)

