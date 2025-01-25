####################################################################################################
# Declare some Makefile variables
####################################################################################################
CC = g++
LANG_STD = -std=gnu++17
COMPILER_FLAGS = -Wall -Wfatal-errors
INCLUDE_PATH = -I"./libs/"
SRC_FILES = ./src/*.cpp \
			./src/Game/*.cpp \
			./src/Logger/*.cpp \
			./src/ECS/*.cpp \
			./src/AssetStore/*.cpp \
			./libs/imgui/*.cpp
LINKER_FLAGS = -lSDL2 -lSDL2_image -lSDL2_mixer -lSDL2_ttf -llua 
OBJ_NAME = gameengine
build:
	$(CC) $(COMPILER_FLAGS) $(LANG_STD) $(INCLUDE_PATH) $(SRC_FILES) $(LINKER_FLAGS) -o $(OBJ_NAME)

run:
	./$(OBJ_NAME)

clean:
	rm $(OBJ_NAME)
