-include .env

# ====================
# Project Configuration
# ====================
PROG        = Final_assignment
SRC_DIR     = src
BUILD_DIR   = build
HEADER_DIRS = include src $(shell find ${SRC_DIR} -type d)

# ====================
# Build Settings
# ====================
export DEBUG ?= 1

# ====================
# Build Directory Setup
# ====================
ifeq ($(DEBUG), 1)
	BUILD_DIR := ${BUILD_DIR}/debug
else
	BUILD_DIR := ${BUILD_DIR}/release
endif

# ====================
# Source Files
# ====================
SRC         = $(wildcard ${SRC_DIR}/*.c)
OBJ         = $(patsubst ${SRC_DIR}/%.c, ${BUILD_DIR}/%.o, ${SRC})
EXECUTABLE  = ${BUILD_DIR}/${PROG}

# ====================
# Toolchain
# ====================
CC          = clang
SIZE        = size

# ====================
# Safety Flags
# ====================
SAFETY_FLAGS  = -ftrapv
SAFETY_FLAGS += -D_FORTIFY_SOURCE=2
SAFETY_FLAGS += -Wstrict-overflow=5
SAFETY_FLAGS += -fno-strict-aliasing

# ====================
# Common Compiler Flags
# ====================
CFLAGS      = -std=c23
CFLAGS     += -Wall -Wextra
CFLAGS     += -Wshadow
CFLAGS     += -Wundef
CFLAGS     += ${SAFETY_FLAGS}

# ====================
# Include Paths
# ====================
CPPFLAGS    = $(addprefix -I,${HEADER_DIRS})
LDFLAGS     = -lm

# ====================
# Debug vs Release Flags
# ====================
ifeq ($(DEBUG), 1)
	CFLAGS  += -g3
	CFLAGS  += -Og
	CFLAGS  += -DDEBUG
	CFLAGS  += -fsanitize=address,undefined
	CFLAGS  += -fstack-protector-strong
	LDFLAGS += -fsanitize=address,undefined
else
	CFLAGS  += -O3 -DNDEBUG
	CFLAGS  += -ffunction-sections -fdata-sections
	CFLAGS  += -flto
	CFLAGS  += -fmerge-all-constants
	CFLAGS  += -fstrict-aliasing
	LDFLAGS += -flto -Wl,--gc-sections
endif

# ====================
# OS-Specific Commands
# ====================
ifeq ($(OS),Windows_NT)
	RM      = del /F /Q
	RMDIR   = rmdir /S /Q
	MKDIR   = mkdir
	EXE_EXT = .exe
else
	RM      = rm -f
	RMDIR   = rm -rf
	MKDIR   = mkdir -p
	EXE_EXT =
endif

EXECUTABLE := ${EXECUTABLE}${EXE_EXT}

# ====================
# Build Targets
# ====================
.PHONY: all clean run debug release size

debug:
	$(MAKE) DEBUG=1 all size

release:
	$(MAKE) DEBUG=0 all size

all: ${EXECUTABLE}
	@echo "Build complete for ${BUILD_DIR}"
	@echo "Debug mode: ${DEBUG}"

${BUILD_DIR}/%.o: ${SRC_DIR}/%.c | ${BUILD_DIR}
	${CC} ${CFLAGS} ${CPPFLAGS} -c $< -o $@

${EXECUTABLE}: ${OBJ} | ${BUILD_DIR}
	${CC} ${CFLAGS} -o $@ ${OBJ} ${LDFLAGS}

${BUILD_DIR}:
	${MKDIR} ${BUILD_DIR}

size: ${EXECUTABLE}
	@echo
	@${SIZE} $<

run: ${EXECUTABLE}
	./${EXECUTABLE}

clean:
	${RMDIR} build
