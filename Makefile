CC := g++
CXX_FLAGS := -std=c++11 -Wall -Wextra -Wno-format
LINK_FLAGS := -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf -lSDL2_image -lcpr -lfmt
RESOURCE_FLAGS := "./res/Resource.res"

TARGET := Overlay
BUILD_DIR := #./
BINARY := ${BUILD_DIR}$(TARGET)
BUILD_TYPE := default

.PHONY: all debug release
all: release

debug: BUILD_TYPE := debug
release: BUILD_TYPE := release

debug: CXX_FLAGS += -g -Og -DDEBUG
release: CXX_FLAGS += -O2 -DNDEBUG -mwindows

debug: build
release: build

# Do not call this rule directly
build:
	$(if $(strip $(subst $(BUILD_TYPE),,default)),,$(error Invalid build type [debug/release]))
	@echo Compiling $(BUILD_TYPE) build...
	$(CC) $(TARGET).cpp $(RESOURCE_FLAGS) $(CXX_FLAGS) $(LINK_FLAGS) -o $(BINARY).exe
	@echo Successfully compiled the $(BUILD_TYPE) build!

# Other rules
clean:
	del /s "$(BINARY)".exe
