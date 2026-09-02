# Gaggiano screen simulator: LVGL + the real UI code + SDL2 on the host.
# Run through ./gg sim. Objects go to sim/build (git-ignored).
ROOT     := ..
LVGL     := $(ROOT)/libraries/lvgl
UI       := $(ROOT)/gaggiano-v2
BUILD    := build
SDL_CFLAGS  := $(shell sdl2-config --cflags)
SDL_LDFLAGS := $(shell sdl2-config --libs)

INC      := -I. -I$(ROOT)/libraries -I$(LVGL) -I$(UI) -I$(ROOT)/libraries/GaggiaProtocol/src
DEFS     := -DLV_CONF_INCLUDE_SIMPLE -DLV_LVGL_H_INCLUDE_SIMPLE '-DLV_MEM_SIZE=(512U*1024U)'
CFLAGS   := -O1 -g $(INC) $(DEFS) $(SDL_CFLAGS)
CXXFLAGS := -std=c++17 -Wall -Wextra $(CFLAGS)

LVGL_SRC := $(shell find $(LVGL)/src -name '*.c')
LVGL_OBJ := $(patsubst $(LVGL)/src/%.c,$(BUILD)/lvgl/%.o,$(LVGL_SRC))

UI_C     := $(UI)/lv_buildUI.c $(wildcard $(UI)/theme.c) $(UI)/logoMainScreen.c
UI_CPP   := $(UI)/sequencer.cpp $(wildcard $(UI)/history.cpp)
SIM_CPP  := main.cpp my_logging.cpp
OBJ      := $(patsubst $(UI)/%.c,$(BUILD)/ui/%.o,$(UI_C)) $(patsubst $(UI)/%.cpp,$(BUILD)/ui/%.o,$(UI_CPP)) \
            $(patsubst %.cpp,$(BUILD)/sim/%.o,$(SIM_CPP))

TARGET := $(BUILD)/gaggiano-sim

.PHONY: all run clean
all: $(TARGET)

run: $(TARGET)
	./$(TARGET)

$(TARGET): $(LVGL_OBJ) $(OBJ)
	$(CXX) $^ -o $@ $(SDL_LDFLAGS)

$(BUILD)/lvgl/%.o: $(LVGL)/src/%.c
	@mkdir -p $(dir $@)
	$(CC) -w $(CFLAGS) -c $< -o $@

$(BUILD)/ui/%.o: $(UI)/%.c
	@mkdir -p $(dir $@)
	$(CC) -Wall $(CFLAGS) -c $< -o $@

$(BUILD)/ui/%.o: $(UI)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD)/sim/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD)
