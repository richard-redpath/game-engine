FLAGS_RELEASE := -O3 -rdynamic -std=gnu11
FLAGS_DEBUG := -O0 -g -DDEBUG -rdynamic -std=gnu11
FLAGS = ${FLAGS_DEBUG}
LIBS := -lGL -lSDL2 -ldl -lm

OUT_DIR := ./bin/debug/
LIB_DIR := ./lib
EXE_FILE_NAME := engine
EXE_PATH = ${OUT_DIR}${EXE_FILE_NAME}

SRC_FILES := main.c entityComponentSystem.c 2dsprites.c

CC := gcc-4.9

debug: rebuild systems

build: create-dirs
	@${CC} ${SRC_FILES} -o ${EXE_PATH} ${LIBS} ${FLAGS}

release: OUT_DIR=./bin/release/
release: FLAGS=${FLAGS_RELEASE}
release: build

rebuild: clean build
rebuild-release: clean release

create-dirs:
	@mkdir -p ${OUT_DIR}
	@mkdir -p ${LIB_DIR}

clean:
	@rm -rf bin lib *.o *~

run-only:
	@./${EXE_PATH} || true

run-release-only: OUT_DIR=./bin/release/
run-release-only: run-only

run: build run-only

run-release: OUT_DIR=./bin/release/
run-release: rebuild-release run-only

systems: create-dirs
	@${CC} -DDEBUG -g -std=gnu11 -c -O0 -fpic entitySystems.c
	@${CC} -shared -g -std=gnu11 -o ${LIB_DIR}/entitySystems.so entitySystems.o -lm
	@rm -f entitySystems.o

systems-asm:
	@${CC} -DDEBUG -g -std=gnu11 -c -O3 -S -fpic -fverbose-asm entitySystems.c

systems-release: create-dirs
	@${CC} -std=gnu11 -c -fpic entitySystems.c
	@${CC} -std=gnu11 -shared -o ${LIB_DIR}/entitySystems.so entitySystems.o -lm
	@rm -f entitySystems.o
