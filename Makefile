# Compiler and flags
CC      := gcc
CFLAGS  := -Wall -Werror
LFLAGS  := -lSDL3
SRC_DIR := src
BUILD_DIR := build

# Find all source files
SRCS := $(wildcard $(SRC_DIR)/*.c)

# Output binary name
TARGET := $(BUILD_DIR)/woodchip

# Default target
all: $(TARGET)

# Link
$(TARGET): $(SRCS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(LFLAGS) $^ -o $@

# Create build directory if it doesn't exist
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)
