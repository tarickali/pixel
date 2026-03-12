################################################################################
# Pixel - 2D Game Engine
################################################################################
CC = g++
STD = -std=c++17
COMPILER_FLAGS = -Wall -Wfatal-errors
INCLUDE_PATH = -I ./libs
LINKER_FLAGS = -l SDL2 -l SDL2_image -l SDL2_ttf -l SDL2_mixer
OBJ_NAME = pixel

SRC_FILES = \
	./src/*.cpp \
	./src/Game/*.cpp \
	./src/Logger/*.cpp \
	./src/ECS/*.cpp \
	./src/AssetStore/*.cpp \
	./libs/imgui/*.cpp \

################################################################################
# Rules
################################################################################
build:
	$(CC) $(COMPILER_FLAGS) $(STD) $(INCLUDE_PATH) $(SRC_FILES) $(LINKER_FLAGS) -o $(OBJ_NAME)

run: build
	./$(OBJ_NAME)

clean:
	rm -f $(OBJ_NAME)

.PHONY: build run clean
